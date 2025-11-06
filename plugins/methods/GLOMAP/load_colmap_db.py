#!/usr/bin/env python3
"""
COLMAP Database Loader

Read COLMAP generated database files, extract feature data, match data and SfM data
Use pycolmap for data reading and processing
"""

import os
import sys
from pathlib import Path
from typing import Dict, List, Tuple, Optional, Any

# Add current directory to Python path to import colmap_pipeline
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

# Setup conda environment - must be done before importing numpy and pycolmap
from colmap_pipeline import setup_conda_environment
if not setup_conda_environment():
    sys.exit(1)

# Import packages that require conda environment after environment setup
try:
    import numpy as np
    import sqlite3
    import struct
    import logging
    from dataclasses import dataclass
    print("✓ Successfully loaded numpy and sqlite3")
except ImportError as e:
    print("❌ Error: Unable to import required modules")
    print(f"   Detailed error: {e}")
    print("Solution: Re-run installation script to add numpy: cd dependencies && ./install_colmap.sh")
    sys.exit(1)

try:
    import pycolmap
    print(f"✓ Successfully loaded pycolmap version: {pycolmap.__version__}")
    
    # Show main modules and features of pycolmap
    print("\n📦 Available pycolmap modules:")
    print("="*50)
    
    # Get all public attributes
    pycolmap_attrs = [attr for attr in dir(pycolmap) if not attr.startswith('_')]
    
    # Categorize display
    classes = []
    functions = []
    modules = []
    others = []
    
    for attr in pycolmap_attrs:
        obj = getattr(pycolmap, attr)
        if hasattr(obj, '__module__') and hasattr(obj, '__name__'):
            if str(type(obj)) == "<class 'type'>":  # class
                classes.append(attr)
            elif callable(obj):  # function
                functions.append(attr)
            elif hasattr(obj, '__path__') or str(type(obj)).find('module') != -1:  # module
                modules.append(attr)
            else:
                others.append(attr)
        else:
            others.append(attr)
    
    if classes:
        print(f"🏗️  Main classes ({len(classes)}):")
        for cls in sorted(classes):
            try:
                doc = getattr(pycolmap, cls).__doc__
                desc = doc.split('\n')[0] if doc else "No description"
                print(f"   • {cls:<20} - {desc[:60]}")
            except:
                print(f"   • {cls}")
    
    if functions:
        print(f"\n🔧 Main functions ({len(functions)}):")
        for func in sorted(functions):
            try:
                doc = getattr(pycolmap, func).__doc__
                desc = doc.split('\n')[0] if doc else "No description"
                print(f"   • {func:<20} - {desc[:60]}")
            except:
                print(f"   • {func}")
    
    if modules:
        print(f"\n📁 Submodules ({len(modules)}):")
        for mod in sorted(modules):
            print(f"   • {mod}")
    
    if others:
        print(f"\n📋 Other attributes ({len(others)}):")
        for other in sorted(others):
            print(f"   • {other}")
    
    print("="*50)
    
    # Check if Database class exists
    if not hasattr(pycolmap, 'Database'):
        print("\n⚠️  Note: pycolmap 0.5.0 version does not have Database class")
        print("   Will use sqlite3 to directly read COLMAP database")
    
except ImportError as e:
    print("❌ Error: Unable to import pycolmap")
    print(f"   Detailed error: {e}")
    sys.exit(1)

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

@dataclass
class ImageInfo:
    """Image information"""
    image_id: int
    name: str
    camera_id: int
    qvec: np.ndarray = None  # quaternion (w, x, y, z)
    tvec: np.ndarray = None  # translation vector (x, y, z)
    point2D_ids: np.ndarray = None  # 2D point ID array
    xys: np.ndarray = None  # 2D coordinate array

@dataclass
class CameraInfo:
    """Camera information"""
    camera_id: int
    model: str
    width: int
    height: int
    params: np.ndarray

@dataclass 
class Point3DInfo:
    """3D point information"""
    point3D_id: int
    xyz: np.ndarray  # 3D coordinates
    rgb: np.ndarray  # RGB color
    error: float     # reprojection error
    track: List[Tuple[int, int]]  # (image_id, point2D_idx)

@dataclass
class FeatureInfo:
    """Feature point information"""
    image_id: int
    keypoints: np.ndarray  # keypoint coordinates (N, 2)
    descriptors: Optional[np.ndarray] = None  # descriptors (N, 128)

@dataclass
class MatchInfo:
    """Match information"""
    image_id1: int
    image_id2: int
    matches: np.ndarray  # match pairs (N, 2)

class ColmapDatabaseLoader:
    """COLMAP database loader"""
    
    def __init__(self, database_path: str, reconstruction_path: Optional[str] = None):
        """
        Initialize loader
        
        Args:
            database_path: COLMAP database file path
            reconstruction_path: reconstruction result path (containing sparse folder or model files)
        """
        self.database_path = Path(database_path)
        self.reconstruction_path = Path(reconstruction_path) if reconstruction_path else None
        
        if not self.database_path.exists():
            raise FileNotFoundError(f"Database file does not exist: {self.database_path}")
        
        # Open database
        try:
            if hasattr(pycolmap, 'Database'):
                # pycolmap version with Database class
                self.database = pycolmap.Database(str(self.database_path))
                self.use_pycolmap_db = True
                logger.info(f"✓ Using pycolmap.Database to open database: {self.database_path}")
            else:
                # pycolmap without Database class, use sqlite3
                self.connection = sqlite3.connect(str(self.database_path))
                self.connection.row_factory = sqlite3.Row
                self.use_pycolmap_db = False
                logger.info(f"✓ Using sqlite3 to open database: {self.database_path}")
        except Exception as e:
            logger.error(f"Unable to open database: {e}")
            raise
        
        # Load reconstruction results (if provided)
        self.reconstruction = None
        if self.reconstruction_path and self.reconstruction_path.exists():
            try:
                # Check if it's a sparse folder
                if self.reconstruction_path.is_dir():
                    model_path = self.reconstruction_path / "0"  # default model 0
                    if model_path.exists():
                        self.reconstruction = pycolmap.Reconstruction(str(model_path))
                    else:
                        self.reconstruction = pycolmap.Reconstruction(str(self.reconstruction_path))
                else:
                    self.reconstruction = pycolmap.Reconstruction(str(self.reconstruction_path))
                
                logger.info(f"✓ Loaded reconstruction results: {self.reconstruction_path}")
            except Exception as e:
                logger.warning(f"Unable to load reconstruction results: {e}")
    
    def _blob_to_array(self, blob, dtype=np.float32, shape=(-1,)):
        """Convert binary blob from database to numpy array"""
        if blob is None:
            return None
        if sys.version_info[0] >= 3:
            return np.fromstring(blob, dtype=dtype).reshape(*shape)
        else:
            return np.frombuffer(blob, dtype=dtype).reshape(*shape)
    
    def _image_ids_to_pair_id(self, image_id1, image_id2):
        """Calculate pair_id for image pair"""
        MAX_IMAGE_ID = 2**31 - 1
        if image_id1 > image_id2:
            image_id1, image_id2 = image_id2, image_id1
        return image_id1 * MAX_IMAGE_ID + image_id2
    
    def _pair_id_to_image_ids(self, pair_id):
        """Parse image IDs from pair_id"""
        MAX_IMAGE_ID = 2**31 - 1
        image_id2 = pair_id % MAX_IMAGE_ID
        image_id1 = (pair_id - image_id2) // MAX_IMAGE_ID
        return int(image_id1), int(image_id2)
    
    def _read_images_sqlite(self) -> Dict[int, ImageInfo]:
        """Read image information using sqlite3"""
        images_info = {}
        cursor = self.connection.cursor()
        cursor.execute("SELECT image_id, name, camera_id FROM images")
        
        for row in cursor.fetchall():
            images_info[row['image_id']] = ImageInfo(
                image_id=row['image_id'],
                name=row['name'],
                camera_id=row['camera_id'],
                qvec=np.array([1.0, 0.0, 0.0, 0.0]),  # default quaternion
                tvec=np.array([0.0, 0.0, 0.0])        # default translation
            )
        
        return images_info
    
    def _read_cameras_sqlite(self) -> Dict[int, CameraInfo]:
        """Read camera information using sqlite3"""
        cameras_info = {}
        cursor = self.connection.cursor()
        cursor.execute("SELECT camera_id, model, width, height, params FROM cameras")
        
        for row in cursor.fetchall():
            # Parse camera parameters (blob -> float64 array)
            params = self._blob_to_array(row['params'], dtype=np.float64)
            
            cameras_info[row['camera_id']] = CameraInfo(
                camera_id=row['camera_id'],
                model=self._get_camera_model_name(row['model']),
                width=row['width'],
                height=row['height'],
                params=params
            )
        
        return cameras_info
    
    def _get_camera_model_name(self, model_id):
        """Get model name from model ID"""
        model_names = {
            0: 'SIMPLE_PINHOLE',
            1: 'PINHOLE', 
            2: 'SIMPLE_RADIAL',
            3: 'RADIAL',
            4: 'OPENCV',
            5: 'OPENCV_FISHEYE',
            6: 'FULL_OPENCV',
            7: 'FOV',
            8: 'SIMPLE_RADIAL_FISHEYE',
            9: 'RADIAL_FISHEYE',
            10: 'THIN_PRISM_FISHEYE'
        }
        return model_names.get(model_id, f'UNKNOWN_{model_id}')
    
    def _read_keypoints_sqlite(self, image_id: int) -> Optional[np.ndarray]:
        """Read keypoints using sqlite3"""
        cursor = self.connection.cursor()
        cursor.execute("SELECT rows, cols, data FROM keypoints WHERE image_id = ?", (image_id,))
        row = cursor.fetchone()
        
        if row is None:
            return None
        
        # Parse keypoint data (blob -> float32 array -> reshape)
        keypoints = self._blob_to_array(row['data'], dtype=np.float32, shape=(-1, row['cols']))
        return keypoints
    
    def _read_descriptors_sqlite(self, image_id: int) -> Optional[np.ndarray]:
        """Read descriptors using sqlite3"""
        cursor = self.connection.cursor()
        cursor.execute("SELECT rows, cols, data FROM descriptors WHERE image_id = ?", (image_id,))
        row = cursor.fetchone()
        
        if row is None:
            return None
        
        # Parse descriptor data (blob -> uint8 array -> reshape)
        descriptors = self._blob_to_array(row['data'], dtype=np.uint8, shape=(-1, row['cols']))
        return descriptors
    
    def _read_matches_sqlite(self, image_id1: int, image_id2: int) -> Optional[np.ndarray]:
        """Read matches using sqlite3"""
        # Calculate pair_id (using correct COLMAP algorithm)
        pair_id = self._image_ids_to_pair_id(image_id1, image_id2)
            
        cursor = self.connection.cursor()
        cursor.execute("SELECT rows, cols, data FROM matches WHERE pair_id = ?", (pair_id,))
        row = cursor.fetchone()
        
        if row is None:
            return None
        
        # Parse match data (blob -> uint32 array -> reshape)
        matches = self._blob_to_array(row['data'], dtype=np.uint32, shape=(-1, 2))
        return matches

    def get_images_info(self) -> Dict[int, ImageInfo]:
        """Get all image information"""
        images_info = {}
        
        if self.reconstruction:
            # Get image information from reconstruction results
            for image_id, image in self.reconstruction.images.items():
                images_info[image_id] = ImageInfo(
                    image_id=image_id,
                    name=image.name,
                    camera_id=image.camera_id,
                    qvec=image.qvec,
                    tvec=image.tvec,
                    point2D_ids=image.point2D_ids,
                    xys=image.points2D
                )
        elif self.use_pycolmap_db:
            # Use pycolmap Database API
            images = self.database.read_all_images()
            for image in images:
                images_info[image.image_id] = ImageInfo(
                    image_id=image.image_id,
                    name=image.name,
                    camera_id=image.camera_id,
                    qvec=np.array([1.0, 0.0, 0.0, 0.0]),  # default quaternion
                    tvec=np.array([0.0, 0.0, 0.0])        # default translation
                )
        else:
            # Read directly using sqlite3
            images_info = self._read_images_sqlite()
        
        logger.info(f"✓ Loaded {len(images_info)} image information")
        return images_info
    
    def get_cameras_info(self) -> Dict[int, CameraInfo]:
        """Get all camera information"""
        cameras_info = {}
        
        if self.reconstruction:
            # Get camera information from reconstruction results
            for camera_id, camera in self.reconstruction.cameras.items():
                cameras_info[camera_id] = CameraInfo(
                    camera_id=camera_id,
                    model=camera.model_name,
                    width=camera.width,
                    height=camera.height,
                    params=camera.params
                )
        elif self.use_pycolmap_db:
            # Use pycolmap Database API
            cameras = self.database.read_all_cameras()
            for camera in cameras:
                cameras_info[camera.camera_id] = CameraInfo(
                    camera_id=camera.camera_id,
                    model=camera.model_name,
                    width=camera.width,
                    height=camera.height,
                    params=camera.params
                )
        else:
            # Read directly using sqlite3
            cameras_info = self._read_cameras_sqlite()
        
        logger.info(f"✓ Loaded {len(cameras_info)} camera information")
        return cameras_info
    
    def get_points3D_info(self) -> Dict[int, Point3DInfo]:
        """Get all 3D point information"""
        points3D_info = {}
        
        if self.reconstruction:
            for point3D_id, point3D in self.reconstruction.points3D.items():
                track = [(track_element.image_id, track_element.point2D_idx) 
                        for track_element in point3D.track.elements]
                
                points3D_info[point3D_id] = Point3DInfo(
                    point3D_id=point3D_id,
                    xyz=point3D.xyz,
                    rgb=point3D.color,
                    error=point3D.error,
                    track=track
                )
            
            logger.info(f"✓ Loaded {len(points3D_info)} 3D points")
        else:
            logger.warning("No reconstruction results, unable to get 3D point information")
        
        return points3D_info
    
    def get_features(self, image_id: int) -> Optional[FeatureInfo]:
        """Get feature points of specified image"""
        try:
            if self.use_pycolmap_db:
                # Use pycolmap Database API
                keypoints = self.database.read_keypoints(image_id)
                descriptors = self.database.read_descriptors(image_id)
            else:
                # Read directly using sqlite3
                keypoints = self._read_keypoints_sqlite(image_id)
                descriptors = self._read_descriptors_sqlite(image_id)
            
            if keypoints is not None:
                return FeatureInfo(
                    image_id=image_id,
                    keypoints=keypoints,
                    descriptors=descriptors
                )
            else:
                return None
        except Exception as e:
            logger.warning(f"Unable to read features for image {image_id}: {e}")
            return None
    
    def get_all_features(self) -> Dict[int, FeatureInfo]:
        """Get feature points of all images"""
        features = {}
        images_info = self.get_images_info()
        
        for image_id in images_info.keys():
            feature_info = self.get_features(image_id)
            if feature_info:
                features[image_id] = feature_info
        
        logger.info(f"✓ Loaded features for {len(features)} images")
        return features
    
    def get_matches(self, image_id1: int, image_id2: int) -> Optional[MatchInfo]:
        """Get matches between two images"""
        try:
            if self.use_pycolmap_db:
                # Use pycolmap Database API
                matches = self.database.read_matches(image_id1, image_id2)
            else:
                # Read directly using sqlite3
                matches = self._read_matches_sqlite(image_id1, image_id2)
            
            if matches is not None and len(matches) > 0:
                return MatchInfo(
                    image_id1=image_id1,
                    image_id2=image_id2,
                    matches=matches
                )
            else:
                return None
        except Exception as e:
            logger.warning(f"Unable to read matches for images {image_id1}-{image_id2}: {e}")
            return None
    
    def get_all_matches(self) -> List[MatchInfo]:
        """Get matches for all image pairs"""
        matches = []
        
        if self.use_pycolmap_db:
            # Use pycolmap Database API
            match_pairs = self.database.read_all_matches()
            
            for (image_id1, image_id2), match_array in match_pairs.items():
                if len(match_array) > 0:
                    matches.append(MatchInfo(
                        image_id1=image_id1,
                        image_id2=image_id2,
                        matches=match_array
                    ))
        else:
            # Read all matches directly using sqlite3
            cursor = self.connection.cursor()
            cursor.execute("SELECT pair_id, rows, cols, data FROM matches WHERE rows > 0")
            
            for row in cursor.fetchall():
                # Parse pair_id to get image_id1 and image_id2 (using correct COLMAP algorithm)
                pair_id = row['pair_id']
                image_id1, image_id2 = self._pair_id_to_image_ids(pair_id)
                
                # Parse match data
                match_array = self._blob_to_array(row['data'], dtype=np.uint32, shape=(-1, 2))
                
                if match_array is not None and len(match_array) > 0:
                    matches.append(MatchInfo(
                        image_id1=image_id1,
                        image_id2=image_id2,
                        matches=match_array
                    ))
        
        logger.info(f"✓ Loaded matches for {len(matches)} image pairs")
        return matches
    
    def print_summary(self):
        """Print database summary information"""
        print("\n" + "="*60)
        print("COLMAP Database Summary")
        print("="*60)
        
        # Basic information
        print(f"Database path: {self.database_path}")
        if self.reconstruction_path:
            print(f"Reconstruction path: {self.reconstruction_path}")
        
        # Image and camera information
        images_info = self.get_images_info()
        cameras_info = self.get_cameras_info()
        print(f"\n📷 Images and cameras:")
        print(f"  - Number of images: {len(images_info)}")
        print(f"  - Number of cameras: {len(cameras_info)}")
        
        # Feature information
        features = self.get_all_features()
        if features:
            total_features = sum(len(f.keypoints) for f in features.values())
            print(f"\n🎯 Feature points:")
            print(f"  - Images with features: {len(features)}")
            print(f"  - Total feature points: {total_features}")
            avg_features = total_features / len(features) if features else 0
            print(f"  - Average feature points: {avg_features:.1f}")
        
        # Match information
        matches = self.get_all_matches()
        if matches:
            total_matches = sum(len(m.matches) for m in matches)
            print(f"\n🔗 Matches:")
            print(f"  - Number of match pairs: {len(matches)}")
            print(f"  - Total match points: {total_matches}")
            avg_matches = total_matches / len(matches) if matches else 0
            print(f"  - Average match points: {avg_matches:.1f}")
        
        # 3D point information (if reconstruction results exist)
        if self.reconstruction:
            points3D = self.get_points3D_info()
            registered_images = len([img for img in images_info.values() 
                                   if img.point2D_ids is not None and len(img.point2D_ids) > 0])
            print(f"\n🌍 3D Reconstruction:")
            print(f"  - Number of 3D points: {len(points3D)}")
            print(f"  - Registered images: {registered_images}")
            
            if points3D:
                errors = [p.error for p in points3D.values()]
                print(f"  - Average reprojection error: {np.mean(errors):.3f}")
        
        print("="*60)

def main():
    """Command line entry function"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='COLMAP database loader and analysis tool',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    
    # Required parameters
    parser.add_argument('--database_path', required=True,
                       help='COLMAP database file path (.db)')
    
    # Optional parameters
    parser.add_argument('--reconstruction_path',
                       help='Reconstruction result path (sparse folder or model folder)')
    parser.add_argument('--summary', action='store_true', default=True,
                       help='Show database summary information')
    
    args = parser.parse_args()
    
    try:
        # Create loader
        loader = ColmapDatabaseLoader(
            database_path=args.database_path,
            reconstruction_path=args.reconstruction_path
        )
        
        # Show summary
        if args.summary:
            loader.print_summary()
        
        print("\n💡 Usage example:")
        print("# Use in Python:")
        print("from load_colmap_db import ColmapDatabaseLoader")
        print("loader = ColmapDatabaseLoader('database.db', 'sparse/0')")
        print("features = loader.get_all_features()")
        print("matches = loader.get_all_matches()")
        print("points3D = loader.get_points3D_info()")
        
    except Exception as e:
        logger.error(f"Program execution failed: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()