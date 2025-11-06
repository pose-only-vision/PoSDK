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
#       from this software without specific blame or promote products derived
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
COLMAP reconstruction model global pose export tool

Read camera pose information from COLMAP reconstruction result files and export to global_poses.txt file
Input can be:
1. Model folder containing cameras.txt/cameras.bin, images.txt/images.bin
2. Database file + sparse reconstruction result folder

Output format:
- First line: number of cameras
- Subsequent lines: image_name R00 R10 R20 R01 R11 R21 R02 R12 R22 tx ty tz
"""

import argparse
import os
import sys
import struct
import collections
import sqlite3

# Add current directory to Python path for importing colmap_pipeline
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

# Setup conda environment - must be done before importing numpy
from colmap_pipeline import setup_conda_environment
if not setup_conda_environment():
    sys.exit(1)

import numpy as np


# COLMAP data structure definitions
BaseImage = collections.namedtuple(
    "Image", ["id", "qvec", "tvec", "camera_id", "name", "xys", "point3D_ids"]
)

class Image(BaseImage):
    def qvec2rotmat(self):
        return qvec2rotmat(self.qvec)


def qvec2rotmat(qvec):
    """Convert quaternion to rotation matrix"""
    return np.array([
        [1 - 2 * qvec[2]**2 - 2 * qvec[3]**2,
         2 * qvec[1] * qvec[2] - 2 * qvec[0] * qvec[3],
         2 * qvec[3] * qvec[1] + 2 * qvec[0] * qvec[2]],
        [2 * qvec[1] * qvec[2] + 2 * qvec[0] * qvec[3],
         1 - 2 * qvec[1]**2 - 2 * qvec[3]**2,
         2 * qvec[2] * qvec[3] - 2 * qvec[0] * qvec[1]],
        [2 * qvec[3] * qvec[1] - 2 * qvec[0] * qvec[2],
         2 * qvec[2] * qvec[3] + 2 * qvec[0] * qvec[1],
         1 - 2 * qvec[1]**2 - 2 * qvec[2]**2]
    ])


def read_next_bytes(fid, num_bytes, format_char_sequence, endian_character="<"):
    """Read and unpack bytes from binary file"""
    data = fid.read(num_bytes)
    return struct.unpack(endian_character + format_char_sequence, data)


def read_images_text(path):
    """Read COLMAP text format images file"""
    images = {}
    with open(path, "r") as fid:
        while True:
            line = fid.readline()
            if not line:
                break
            line = line.strip()
            if len(line) > 0 and line[0] != "#":
                elems = line.split()
                image_id = int(elems[0])
                qvec = np.array(tuple(map(float, elems[1:5])))
                tvec = np.array(tuple(map(float, elems[5:8])))
                camera_id = int(elems[8])
                image_name = elems[9]
                elems = fid.readline().split()
                xys = np.column_stack([
                    tuple(map(float, elems[0::3])),
                    tuple(map(float, elems[1::3]))
                ])
                point3D_ids = np.array(tuple(map(int, elems[2::3])))
                images[image_id] = Image(
                    id=image_id, qvec=qvec, tvec=tvec,
                    camera_id=camera_id, name=image_name,
                    xys=xys, point3D_ids=point3D_ids
                )
    return images


def read_images_binary(path_to_model_file):
    """Read COLMAP binary format images file"""
    images = {}
    with open(path_to_model_file, "rb") as fid:
        num_reg_images = read_next_bytes(fid, 8, "Q")[0]
        for _ in range(num_reg_images):
            binary_image_properties = read_next_bytes(
                fid, num_bytes=64, format_char_sequence="idddddddi"
            )
            image_id = binary_image_properties[0]
            qvec = np.array(binary_image_properties[1:5])
            tvec = np.array(binary_image_properties[5:8])
            camera_id = binary_image_properties[8]
            binary_image_name = b""
            current_char = read_next_bytes(fid, 1, "c")[0]
            while current_char != b"\x00":
                binary_image_name += current_char
                current_char = read_next_bytes(fid, 1, "c")[0]
            image_name = binary_image_name.decode("utf-8")
            num_points2D = read_next_bytes(fid, num_bytes=8, format_char_sequence="Q")[0]
            x_y_id_s = read_next_bytes(
                fid, num_bytes=24 * num_points2D, 
                format_char_sequence="ddq" * num_points2D
            )
            xys = np.column_stack([
                tuple(map(float, x_y_id_s[0::3])),
                tuple(map(float, x_y_id_s[1::3]))
            ])
            point3D_ids = np.array(tuple(map(int, x_y_id_s[2::3])))
            images[image_id] = Image(
                id=image_id, qvec=qvec, tvec=tvec,
                camera_id=camera_id, name=image_name,
                xys=xys, point3D_ids=point3D_ids
            )
    return images


def detect_model_format(path, ext):
    """Detect model file format"""
    return (os.path.isfile(os.path.join(path, f"cameras{ext}")) and 
            os.path.isfile(os.path.join(path, f"images{ext}")))


def read_images_from_model(model_path):
    """Read images from COLMAP model folder"""
    print("🎯 Reading image poses from model files...")
    
    # Auto-detect format
    if detect_model_format(model_path, ".bin"):
        ext = ".bin"
        print("   Detected binary format (.bin)")
    elif detect_model_format(model_path, ".txt"):
        ext = ".txt"
        print("   Detected text format (.txt)")
    else:
        raise ValueError(f"No valid COLMAP model files found in {model_path}")
    
    images_path = os.path.join(model_path, f"images{ext}")
    
    if ext == ".txt":
        images = read_images_text(images_path)
    else:
        images = read_images_binary(images_path)
    
    print(f"   Successfully read pose information for {len(images)} images")
    return images


def find_sparse_model(base_path):
    """Find sparse reconstruction result folder"""
    possible_paths = [
        os.path.join(base_path, "sparse", "0"),
        os.path.join(base_path, "sparse"),
        base_path
    ]
    
    for path in possible_paths:
        if os.path.exists(path):
            if detect_model_format(path, ".bin") or detect_model_format(path, ".txt"):
                return path
    
    return None


def export_global_poses_txt(images, output_path):
    """Export to global_poses.txt format"""
    with open(output_path, 'w') as f:
        # First line: number of cameras
        f.write(f"{len(images)}\n")
        
        # Each line: image_name + rotation matrix (column-major) + translation vector
        for image in images.values():
            R = image.qvec2rotmat()
            tx, ty, tz = image.tvec
            
            # Output rotation matrix in column-major order: R(0,0) R(1,0) R(2,0) R(0,1) R(1,1) R(2,1) R(0,2) R(1,2) R(2,2)
            f.write(f"{image.name} "
                   f"{R[0,0]:.8f} {R[1,0]:.8f} {R[2,0]:.8f} "
                   f"{R[0,1]:.8f} {R[1,1]:.8f} {R[2,1]:.8f} "
                   f"{R[0,2]:.8f} {R[1,2]:.8f} {R[2,2]:.8f} "
                   f"{tx:.8f} {ty:.8f} {tz:.8f}\n")


def main():
    parser = argparse.ArgumentParser(
        description="Export global poses from COLMAP model files to global_poses.txt",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    
    # Input parameters - support multiple input methods
    input_group = parser.add_mutually_exclusive_group(required=True)
    input_group.add_argument('--model_folder', 
                           help='COLMAP model folder path (containing cameras.txt/bin, images.txt/bin)')
    input_group.add_argument('--workspace_path',
                           help='COLMAP workspace path (automatically find sparse reconstruction results)')
    
    # Output parameters
    parser.add_argument('--output_folder', required=True,
                       help='Output folder path')
    
    args = parser.parse_args()
    
    try:
        print("="*60)
        print("COLMAP Global Pose Export Tool")
        print("="*60)
        print(f"Output folder: {args.output_folder}")
        
        # Determine model path
        if args.model_folder:
            model_path = args.model_folder
            print(f"Model path: {model_path}")
        else:
            print(f"Workspace path: {args.workspace_path}")
            model_path = find_sparse_model(args.workspace_path)
            if model_path is None:
                print("❌ Error: Cannot find sparse reconstruction results in workspace")
                sys.exit(1)
            print(f"Found model path: {model_path}")
        
        # Check model path
        if not os.path.exists(model_path):
            print(f"❌ Error: Model path does not exist: {model_path}")
            sys.exit(1)
        
        print()
        
        # Create output folder
        os.makedirs(args.output_folder, exist_ok=True)
        
        # Read image poses
        images = read_images_from_model(model_path)
        
        if len(images) == 0:
            print("⚠️  Warning: No image pose data found")
            return
        
        # Export global_poses.txt file
        output_path = os.path.join(args.output_folder, "global_poses.txt")
        export_global_poses_txt(images, output_path)
        
        print(f"✅ Global poses exported: {output_path}")
        
        # Statistics
        print(f"\n📊 Statistics:")
        print(f"   Number of images: {len(images)}")
        
        # Show first few image information
        print(f"\n📋 Sample data:")
        for i, (_, image) in enumerate(images.items()):
            if i >= 3:  # Only show first 3
                break
            print(f"   {image.name}: camera_id={image.camera_id}")
        
        print(f"\n💡 Output format description:")
        print(f"   First line: number of cameras")
        print(f"   Subsequent lines: image_name R00 R10 R20 R01 R11 R21 R02 R12 R22 tx ty tz")
        print(f"   Where R is rotation matrix (column-major), t is translation vector")
        
    except Exception as e:
        print(f"❌ Program execution failed: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()