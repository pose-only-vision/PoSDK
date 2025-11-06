#!/usr/bin/env python3

# Copyright (c), ETH Zurich and UNC Chapel Hill.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#
#     * Neither the name of ETH Zurich and UNC Chapel Hill nor the names of
#       its contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

"""
COLMAP Database Camera Pose Export Tool

Read camera pose information from COLMAP database and export to readable formats
Supported output formats:
- TXT format: Each line contains image name, rotation quaternion, translation vector
- CSV format: Convenient for analysis in Excel or other tools
- JSON format: Structured data, convenient for program processing
"""

import argparse
import json
import os
import sqlite3
import sys
import math

# Add current directory to Python path for importing colmap_pipeline
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

# Setup conda environment - must be done before importing numpy
from colmap_pipeline import setup_conda_environment
if not setup_conda_environment():
    sys.exit(1)

import numpy as np


def quaternion_to_rotation_matrix(qw, qx, qy, qz):
    """Convert quaternion to rotation matrix"""
    # Normalize quaternion
    norm = math.sqrt(qw*qw + qx*qx + qy*qy + qz*qz)
    qw, qx, qy, qz = qw/norm, qx/norm, qy/norm, qz/norm
    
    # Build rotation matrix
    R = np.array([
        [1 - 2*(qy*qy + qz*qz), 2*(qx*qy - qw*qz), 2*(qx*qz + qw*qy)],
        [2*(qx*qy + qw*qz), 1 - 2*(qx*qx + qz*qz), 2*(qy*qz - qw*qx)],
        [2*(qx*qz - qw*qy), 2*(qy*qz + qw*qx), 1 - 2*(qx*qx + qy*qy)]
    ])
    
    return R


def rotation_matrix_to_euler_angles(R):
    """Convert rotation matrix to Euler angles (roll, pitch, yaw) in degrees"""
    # Use ZYX order Euler angles
    sy = math.sqrt(R[0,0]*R[0,0] + R[1,0]*R[1,0])
    
    singular = sy < 1e-6
    
    if not singular:
        x = math.atan2(R[2,1], R[2,2])  # roll
        y = math.atan2(-R[2,0], sy)     # pitch
        z = math.atan2(R[1,0], R[0,0])  # yaw
    else:
        x = math.atan2(-R[1,2], R[1,1])  # roll
        y = math.atan2(-R[2,0], sy)      # pitch
        z = 0                            # yaw
    
    # Convert to degrees
    return np.degrees([x, y, z])


def read_camera_poses_from_database(database_path):
    """Read camera pose information from COLMAP database"""
    connection = sqlite3.connect(database_path)
    cursor = connection.cursor()
    
    # Read camera information
    print("📷 Reading camera information...")
    cursor.execute("SELECT camera_id, model, width, height, params FROM cameras")
    cameras = {}
    for row in cursor.fetchall():
        camera_id, model, width, height, params = row
        # Parse camera parameters
        if sys.version_info[0] >= 3:
            param_array = np.frombuffer(params, dtype=np.float64) if params else np.array([])
        else:
            param_array = np.fromstring(params, dtype=np.float64) if params else np.array([])
        
        cameras[camera_id] = {
            'model': model,
            'width': width,
            'height': height,
            'params': param_array
        }
    print(f"   Found {len(cameras)} cameras")
    
    # Read image pose information
    print("🎯 Reading image pose information...")
    cursor.execute("""
        SELECT image_id, name, camera_id, 
               prior_qw, prior_qx, prior_qy, prior_qz, 
               prior_tx, prior_ty, prior_tz,
               qw, qx, qy, qz, tx, ty, tz
        FROM images
    """)
    
    poses_data = []
    registered_count = 0
    
    for row in cursor.fetchall():
        (image_id, name, camera_id, 
         prior_qw, prior_qx, prior_qy, prior_qz,
         prior_tx, prior_ty, prior_tz,
         qw, qx, qy, qz, tx, ty, tz) = row
        
        # Check if image is registered (has optimized pose)
        is_registered = (qw is not None and qx is not None and 
                        qy is not None and qz is not None and
                        tx is not None and ty is not None and tz is not None)
        
        if is_registered:
            registered_count += 1
            
        # Use optimized pose if available, otherwise use prior pose
        if is_registered:
            final_qw, final_qx, final_qy, final_qz = qw, qx, qy, qz
            final_tx, final_ty, final_tz = tx, ty, tz
        elif (prior_qw is not None and prior_qx is not None and 
              prior_qy is not None and prior_qz is not None):
            final_qw, final_qx, final_qy, final_qz = prior_qw, prior_qx, prior_qy, prior_qz
            final_tx, final_ty, final_tz = prior_tx or 0, prior_ty or 0, prior_tz or 0
        else:
            # Skip if no pose information available
            continue
        
        # Calculate rotation matrix and Euler angles
        try:
            R = quaternion_to_rotation_matrix(final_qw, final_qx, final_qy, final_qz)
            euler_angles = rotation_matrix_to_euler_angles(R)
            
            # Calculate camera center position (world coordinate system)
            # t in COLMAP is translation from world to camera coordinate system
            # Camera center position = -R^T * t
            camera_center = -R.T @ np.array([final_tx, final_ty, final_tz])
            
            pose_data = {
                'image_id': image_id,
                'image_name': name,
                'camera_id': camera_id,
                'is_registered': is_registered,
                # Quaternion (w, x, y, z)
                'quaternion': [final_qw, final_qx, final_qy, final_qz],
                # Translation vector (world to camera)
                'translation': [final_tx, final_ty, final_tz],
                # Camera center position (world coordinate system)
                'camera_center': camera_center.tolist(),
                # Euler angles (roll, pitch, yaw) in degrees
                'euler_angles': euler_angles.tolist(),
                # Rotation matrix
                'rotation_matrix': R.tolist()
            }
            
            poses_data.append(pose_data)
            
        except Exception as e:
            print(f"   Warning: Error processing pose for image {name}: {e}")
            continue
    
    print(f"   Successfully read pose information for {len(poses_data)} images")
    print(f"   {registered_count} images are registered")
    
    connection.close()
    return poses_data, cameras


def export_poses_txt(poses_data, output_path):
    """Export to TXT format"""
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write("# COLMAP Camera Pose Export File\n")
        f.write("# Format: image_name registration_status quaternion(w,x,y,z) translation(x,y,z) camera_center(x,y,z) euler_angles(roll,pitch,yaw)\n")
        f.write("# Registration status: 1=registered, 0=prior only\n")
        f.write("#\n")
        
        for pose in poses_data:
            qw, qx, qy, qz = pose['quaternion']
            tx, ty, tz = pose['translation']
            cx, cy, cz = pose['camera_center']
            roll, pitch, yaw = pose['euler_angles']
            
            f.write(f"{pose['image_name']} {int(pose['is_registered'])} "
                   f"{qw:.8f} {qx:.8f} {qy:.8f} {qz:.8f} "
                   f"{tx:.8f} {ty:.8f} {tz:.8f} "
                   f"{cx:.8f} {cy:.8f} {cz:.8f} "
                   f"{roll:.6f} {pitch:.6f} {yaw:.6f}\n")


def export_poses_csv(poses_data, output_path):
    """Export to CSV format"""
    with open(output_path, 'w', encoding='utf-8') as f:
        # CSV header
        f.write("image_name,camera_id,is_registered,"
               "qw,qx,qy,qz,tx,ty,tz,"
               "camera_center_x,camera_center_y,camera_center_z,"
               "roll_deg,pitch_deg,yaw_deg\n")
        
        for pose in poses_data:
            qw, qx, qy, qz = pose['quaternion']
            tx, ty, tz = pose['translation']
            cx, cy, cz = pose['camera_center']
            roll, pitch, yaw = pose['euler_angles']
            
            f.write(f"{pose['image_name']},{pose['camera_id']},{int(pose['is_registered'])},"
                   f"{qw:.8f},{qx:.8f},{qy:.8f},{qz:.8f},"
                   f"{tx:.8f},{ty:.8f},{tz:.8f},"
                   f"{cx:.8f},{cy:.8f},{cz:.8f},"
                   f"{roll:.6f},{pitch:.6f},{yaw:.6f}\n")


def export_poses_json(poses_data, cameras, output_path):
    """Export to JSON format"""
    output_data = {
        'cameras': cameras,
        'poses': poses_data,
        'metadata': {
            'total_images': len(poses_data),
            'registered_images': sum(1 for p in poses_data if p['is_registered']),
            'coordinate_system': 'COLMAP (right-handed coordinate system)',
            'quaternion_format': 'w,x,y,z',
            'translation_description': 'Translation from world coordinate system to camera coordinate system',
            'camera_center_description': 'Camera position in world coordinate system',
            'euler_angles_unit': 'degrees',
            'euler_angles_order': 'ZYX (roll, pitch, yaw)'
        }
    }
    
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(output_data, f, ensure_ascii=False, indent=2)


def main():
    parser = argparse.ArgumentParser(
        description="Export camera pose information from COLMAP database",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    
    # Required arguments
    parser.add_argument('--database_path', required=True,
                       help='Path to COLMAP database file (.db)')
    parser.add_argument('--output_folder', required=True,
                       help='Output folder path')
    
    # Optional arguments
    parser.add_argument('--format', choices=['txt', 'csv', 'json', 'all'], 
                       default='all', help='Output format')
    parser.add_argument('--registered_only', action='store_true',
                       help='Export only registered images')
    
    args = parser.parse_args()
    
    # Check input
    if not os.path.exists(args.database_path):
        print(f"❌ Error: Database file does not exist: {args.database_path}")
        sys.exit(1)
    
    try:
        print("="*60)
        print("COLMAP Camera Pose Export Tool")
        print("="*60)
        print(f"Database path: {args.database_path}")
        print(f"Output folder: {args.output_folder}")
        print(f"Output format: {args.format}")
        print(f"Registered only: {args.registered_only}")
        print()
        
        # Create output folder
        os.makedirs(args.output_folder, exist_ok=True)
        
        # Read pose data
        poses_data, cameras = read_camera_poses_from_database(args.database_path)
        
        if len(poses_data) == 0:
            print("⚠️  Warning: No pose data found")
            return
        
        # Filter to registered images only
        if args.registered_only:
            filtered_poses = [p for p in poses_data if p['is_registered']]
            print(f"🔍 Filtered to keep {len(filtered_poses)}/{len(poses_data)} registered images")
            poses_data = filtered_poses
        
        if len(poses_data) == 0:
            print("⚠️  Warning: No image data after filtering")
            return
        
        # Export files
        base_name = "camera_poses"
        
        if args.format in ['txt', 'all']:
            txt_path = os.path.join(args.output_folder, f"{base_name}.txt")
            export_poses_txt(poses_data, txt_path)
            print(f"✅ TXT format exported: {txt_path}")
        
        if args.format in ['csv', 'all']:
            csv_path = os.path.join(args.output_folder, f"{base_name}.csv")
            export_poses_csv(poses_data, csv_path)
            print(f"✅ CSV format exported: {csv_path}")
        
        if args.format in ['json', 'all']:
            json_path = os.path.join(args.output_folder, f"{base_name}.json")
            export_poses_json(poses_data, cameras, json_path)
            print(f"✅ JSON format exported: {json_path}")
        
        # Statistics
        registered_count = sum(1 for p in poses_data if p['is_registered'])
        
        print(f"\n📊 Statistics:")
        print(f"   Total images: {len(poses_data)}")
        print(f"   Registered images: {registered_count}")
        print(f"   Cameras: {len(cameras)}")
        
        if registered_count > 0:
            # Calculate camera position range
            centers = np.array([p['camera_center'] for p in poses_data if p['is_registered']])
            if len(centers) > 0:
                center_min = centers.min(axis=0)
                center_max = centers.max(axis=0)
                center_range = center_max - center_min
                
                print(f"\n📍 Camera position range (registered images):")
                print(f"   X: [{center_min[0]:.3f}, {center_max[0]:.3f}] (range: {center_range[0]:.3f})")
                print(f"   Y: [{center_min[1]:.3f}, {center_max[1]:.3f}] (range: {center_range[1]:.3f})")
                print(f"   Z: [{center_min[2]:.3f}, {center_max[2]:.3f}] (range: {center_range[2]:.3f})")
        
        print(f"\n💡 Usage instructions:")
        print(f"   - Quaternion format: (w, x, y, z)")
        print(f"   - Coordinate system: COLMAP right-handed coordinate system")
        print(f"   - Translation vector: From world coordinate system to camera coordinate system")
        print(f"   - Camera center: Camera position in world coordinate system")
        print(f"   - Euler angles: ZYX order (roll, pitch, yaw), unit in degrees")
        
    except Exception as e:
        print(f"❌ Program execution failed: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main() 