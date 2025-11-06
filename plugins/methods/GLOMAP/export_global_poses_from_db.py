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
COLMAP database global pose export tool

Reads camera pose information from COLMAP database and exports to global_pose.txt file
Output format:
- First line: Number of cameras
- Subsequent lines: Image_name R00 R10 R20 R01 R11 R21 R02 R12 R22 tx ty tz
"""

import argparse
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


def read_global_poses_from_database(database_path):
    """Read global pose information from COLMAP database"""
    connection = sqlite3.connect(database_path)
    cursor = connection.cursor()
    
    # First check database structure
    print("🔍 Checking database structure...")
    cursor.execute("PRAGMA table_info(images)")
    columns = [row[1] for row in cursor.fetchall()]
    print(f"   images table contains columns: {columns}")
    
    # Check if quaternion and translation data exist
    has_prior = all(col in columns for col in ['prior_qw', 'prior_qx', 'prior_qy', 'prior_qz'])
    has_optimized = all(col in columns for col in ['qw', 'qx', 'qy', 'qz', 'tx', 'ty', 'tz'])
    
    print(f"   Prior poses available: {has_prior}")
    print(f"   Optimized poses available: {has_optimized}")
    
    if not has_prior and not has_optimized:
        print("❌ Error: No pose information found in database")
        connection.close()
        return []
    
    # Read image pose information
    print("🎯 Reading image pose information...")
    
    if has_optimized:
        # Prefer optimized poses
        cursor.execute("""
            SELECT image_id, name, camera_id, 
                   qw, qx, qy, qz, tx, ty, tz
            FROM images
            WHERE qw IS NOT NULL AND qx IS NOT NULL AND qy IS NOT NULL AND qz IS NOT NULL
            AND tx IS NOT NULL AND ty IS NOT NULL AND tz IS NOT NULL
        """)
        print("   Using optimized pose data")
    elif has_prior:
        # Use prior poses
        cursor.execute("""
            SELECT image_id, name, camera_id, 
                   prior_qw, prior_qx, prior_qy, prior_qz, 
                   COALESCE(prior_tx, 0), COALESCE(prior_ty, 0), COALESCE(prior_tz, 0)
            FROM images
            WHERE prior_qw IS NOT NULL AND prior_qx IS NOT NULL 
            AND prior_qy IS NOT NULL AND prior_qz IS NOT NULL
        """)
        print("   Using prior pose data")
    
    poses_data = []
    
    for row in cursor.fetchall():
        (image_id, name, camera_id, qw, qx, qy, qz, tx, ty, tz) = row
        
        try:
            # Calculate rotation matrix
            R = quaternion_to_rotation_matrix(qw, qx, qy, qz)
            
            pose_data = {
                'image_id': image_id,
                'image_name': name,
                'camera_id': camera_id,
                'rotation_matrix': R,
                'translation': [tx, ty, tz]
            }
            
            poses_data.append(pose_data)
            
        except Exception as e:
            print(f"   Warning: Error processing pose for image {name}: {e}")
            continue
    
    print(f"   Successfully read pose information for {len(poses_data)} images")
    
    connection.close()
    return poses_data


def export_global_poses_txt(poses_data, output_path):
    """Export to global_pose.txt format"""
    with open(output_path, 'w') as f:
        # First line: Number of cameras
        f.write(f"{len(poses_data)}\n")
        
        # Each line: Image name + rotation matrix (column-major) + translation vector
        for pose in poses_data:
            R = pose['rotation_matrix']
            tx, ty, tz = pose['translation']
            
            # Output rotation matrix in column-major order: R(0,0) R(1,0) R(2,0) R(0,1) R(1,1) R(2,1) R(0,2) R(1,2) R(2,2)
            f.write(f"{pose['image_name']} "
                   f"{R[0,0]:.8f} {R[1,0]:.8f} {R[2,0]:.8f} "
                   f"{R[0,1]:.8f} {R[1,1]:.8f} {R[2,1]:.8f} "
                   f"{R[0,2]:.8f} {R[1,2]:.8f} {R[2,2]:.8f} "
                   f"{tx:.8f} {ty:.8f} {tz:.8f}\n")


def main():
    parser = argparse.ArgumentParser(
        description="Export global poses from COLMAP database to global_pose.txt",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    
    # Required arguments
    parser.add_argument('--database_path', required=True,
                       help='COLMAP database file path (.db)')
    parser.add_argument('--output_folder', required=True,
                       help='Output folder path')
    
    args = parser.parse_args()
    
    # Check input
    if not os.path.exists(args.database_path):
        print(f"❌ Error: Database file does not exist: {args.database_path}")
        sys.exit(1)
    
    try:
        print("="*60)
        print("COLMAP Global Pose Export Tool")
        print("="*60)
        print(f"Database path: {args.database_path}")
        print(f"Output folder: {args.output_folder}")
        print()
        
        # Create output folder
        os.makedirs(args.output_folder, exist_ok=True)
        
        # Read pose data (only registered images)
        poses_data = read_global_poses_from_database(args.database_path)
        
        if len(poses_data) == 0:
            print("⚠️  Warning: No registered image pose data found")
            return
        
        # Export global_pose.txt file
        output_path = os.path.join(args.output_folder, "global_pose.txt")
        export_global_poses_txt(poses_data, output_path)
        
        print(f"✅ Global poses exported: {output_path}")
        
        # Statistics
        print(f"\n📊 Statistics:")
        print(f"   Registered images: {len(poses_data)}")
        
        print(f"\n💡 Output format description:")
        print(f"   First line: Number of cameras")
        print(f"   Subsequent lines: Image_name R00 R10 R20 R01 R11 R21 R02 R12 R22 tx ty tz")
        print(f"   Where R is rotation matrix (column-major), t is translation vector")
        
    except Exception as e:
        print(f"❌ Program execution failed: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main() 