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
COLMAP database matches export tool

Read match data from COLMAP database and export to match files
Each match file format:
- First line: camera_id1 camera_id2
- Following lines: X1 Y1 X2 Y2 (image coordinates of matching points)
"""

import argparse
import os
import sqlite3
import sys

# Add current directory to Python path for importing colmap_pipeline
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

# Setup conda environment - must be done before importing numpy and pycolmap
from colmap_pipeline import setup_conda_environment
if not setup_conda_environment():
    sys.exit(1)

import numpy as np


def pair_id_to_image_ids(pair_id):
    """Parse image ID pair from pair_id"""
    MAX_IMAGE_ID = 2**31 - 1
    image_id2 = pair_id % MAX_IMAGE_ID
    image_id1 = (pair_id - image_id2) // MAX_IMAGE_ID
    return int(image_id1), int(image_id2)


def blob_to_array(blob, dtype, shape=(-1,)):
    """Convert binary blob from database to numpy array"""
    if blob is None:
        return None
    if sys.version_info[0] >= 3:
        return np.frombuffer(blob, dtype=dtype).reshape(*shape)
    else:
        return np.fromstring(blob, dtype=dtype).reshape(*shape)


def read_database_matches(database_path):
    """Read all match data from COLMAP database"""
    connection = sqlite3.connect(database_path)
    cursor = connection.cursor()
    
    # Read image information (image_id -> camera_id, name)
    print("📷 Reading image information...")
    cursor.execute("SELECT image_id, name, camera_id FROM images")
    images = {}
    for row in cursor.fetchall():
        image_id, name, camera_id = row
        images[image_id] = {
            'name': name,
            'camera_id': camera_id
        }
    print(f"   Found {len(images)} images")
    
    # Read keypoints data (image_id -> keypoints)
    print("🎯 Reading keypoints data...")
    cursor.execute("SELECT image_id, rows, cols, data FROM keypoints")
    keypoints = {}
    for row in cursor.fetchall():
        image_id, rows, cols, data = row
        if data is not None:
            kpts = blob_to_array(data, np.float32, (-1, cols))
            keypoints[image_id] = kpts
    print(f"   Found keypoints for {len(keypoints)} images")
    
    # Read match data
    print("🔗 Reading match data...")
    cursor.execute("SELECT pair_id, rows, cols, data FROM matches WHERE rows > 0")
    matches_data = []
    
    for row in cursor.fetchall():
        pair_id, rows, cols, data = row
        if data is not None and rows > 0:
            # Parse image pair
            image_id1, image_id2 = pair_id_to_image_ids(pair_id)
            
            # Ensure both images have information
            if image_id1 not in images or image_id2 not in images:
                continue
                
            # Ensure both images have keypoints
            if image_id1 not in keypoints or image_id2 not in keypoints:
                continue
            
            # Parse match data
            matches = blob_to_array(data, np.uint32, (-1, 2))
            if matches is None or len(matches) == 0:
                continue
            
            # Get coordinates of matching points
            kpts1 = keypoints[image_id1]
            kpts2 = keypoints[image_id2]
            
            match_coords = []
            for match in matches:
                idx1, idx2 = match
                if idx1 < len(kpts1) and idx2 < len(kpts2):
                    x1, y1 = kpts1[idx1][:2]  # Only take x,y coordinates
                    x2, y2 = kpts2[idx2][:2]
                    match_coords.append([x1, y1, x2, y2])
            
            if len(match_coords) > 0:
                matches_data.append({
                    'image_id1': image_id1,
                    'image_id2': image_id2,
                    'camera_id1': images[image_id1]['camera_id'],
                    'camera_id2': images[image_id2]['camera_id'],
                    'image_name1': images[image_id1]['name'],
                    'image_name2': images[image_id2]['name'],
                    'matches': np.array(match_coords)
                })
    
    print(f"   Found {len(matches_data)} valid match pairs")
    
    connection.close()
    return matches_data


def export_matches_to_files(matches_data, output_folder):
    """Export match data to files"""
    os.makedirs(output_folder, exist_ok=True)
    
    print(f"\n Exporting match files to: {output_folder}")
    
    for i, match_data in enumerate(matches_data):
        # Generate filename (using image names, removing extensions)
        name1 = os.path.splitext(match_data['image_name1'])[0]
        name2 = os.path.splitext(match_data['image_name2'])[0]
        filename = f"matches_{name1}_{name2}.txt"
        filepath = os.path.join(output_folder, filename)
        
        # Write match file
        with open(filepath, 'w') as f:
            # First line: camera_id1 camera_id2
            f.write(f"{match_data['camera_id1']} {match_data['camera_id2']}\n")
            
            # Following lines: X1 Y1 X2 Y2
            for match in match_data['matches']:
                x1, y1, x2, y2 = match
                f.write(f"{x1:.6f} {y1:.6f} {x2:.6f} {y2:.6f}\n")
        
        print(f"   [{i+1:3d}/{len(matches_data)}] {filename} - {len(match_data['matches'])} match points")
    
    print(f"\n Successfully exported {len(matches_data)} match files")


def main():
    parser = argparse.ArgumentParser(
        description="Export match data from COLMAP database",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    
    # Required arguments
    parser.add_argument('--database_path', required=True,
                       help='COLMAP database file path (.db)')
    parser.add_argument('--output_folder', required=True,
                       help='Output folder path')
    
    # Optional arguments
    parser.add_argument('--min_matches', type=int, default=15,
                       help='Minimum number of matches threshold')
    
    args = parser.parse_args()
    
    # Check input
    if not os.path.exists(args.database_path):
        print(f"❌ Error: Database file does not exist: {args.database_path}")
        sys.exit(1)
    
    try:
        print("="*60)
        print("COLMAP Database Matches Export Tool")
        print("="*60)
        print(f"Database path: {args.database_path}")
        print(f"Output folder: {args.output_folder}")
        print(f"Minimum matches: {args.min_matches}")
        print()
        
        # Read match data
        matches_data = read_database_matches(args.database_path)
        
        # Filter by match count
        filtered_matches = [m for m in matches_data if len(m['matches']) >= args.min_matches]
        if len(filtered_matches) < len(matches_data):
            print(f"🔍 Filtered to keep {len(filtered_matches)}/{len(matches_data)} match pairs (>= {args.min_matches} match points)")
        
        if len(filtered_matches) == 0:
            print("⚠️  Warning: No match data found meeting the criteria")
            return
        
        # Export files
        export_matches_to_files(filtered_matches, args.output_folder)
        
        # Statistics
        total_matches = sum(len(m['matches']) for m in filtered_matches)
        avg_matches = total_matches / len(filtered_matches)
        
        print(f"\n📊 Statistics:")
        print(f"   Total match pairs: {len(filtered_matches)}")
        print(f"   Total match points: {total_matches}")
        print(f"   Average match points: {avg_matches:.1f}")
        
        print("\n💡 Usage example:")
        print("   # Each match file format:")
        print("   # First line: camera_id1 camera_id2")
        print("   # Following lines: X1 Y1 X2 Y2")
        
    except Exception as e:
        print(f"❌ Program execution failed: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main() 