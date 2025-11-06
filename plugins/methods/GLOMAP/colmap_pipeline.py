#!/usr/bin/env python3
#use colmap to do the pipeline

import os
import sys
import shutil
import logging
from pathlib import Path
from typing import Optional, Dict, Any

def setup_conda_environment():
    """
    Automatically setup and activate conda environment
    
    Returns:
        Whether environment setup was successful
    """
    # Get project root directory and dependencies directory
    script_dir = Path(__file__).parent.absolute()
    project_dir = script_dir.parent.parent.parent  # Fix: need to go up 3 levels to reach src directory
    dependencies_dir = project_dir / "dependencies"
    
    # Search for possible conda installation paths
    possible_conda_dirs = [
        dependencies_dir / "miniforge3",         # Project local miniforge3 installation
        Path.home() / "miniconda3",             # User home miniconda3
        Path.home() / "miniforge3",             # User home miniforge3
        Path.home() / "anaconda3",              # User home anaconda3
        Path("/opt/miniconda3"),                # System-level miniconda3
        Path("/opt/miniforge3"),                # System-level miniforge3
    ]
    
    miniforge_dir = None
    for conda_dir in possible_conda_dirs:
        if conda_dir.exists():
            miniforge_dir = conda_dir
            break
    
    # Check if conda installation was found
    if not miniforge_dir:
        print("❌ Error: No conda installation found")
        print("   Searched paths:")
        for conda_dir in possible_conda_dirs:
            print(f"   - {conda_dir}")
        print("\nSolutions:")
        print("1. Run installation script: cd dependencies && ./install_colmap.sh")
        print("2. Or manual installation: ./dependencies/install_colmap.sh")
        return False
    
    print(f"✓ Found conda installation: {miniforge_dir}")
    
    # Check if pycolmap_env environment exists
    env_dir = miniforge_dir / "envs" / "pycolmap_env"
    if not env_dir.exists():
        print("❌ Error: pycolmap_env environment does not exist")
        print(f"   Searched path: {env_dir}")
        print("\nSolution: Re-run installation script")
        return False
    
    # Get Python path in conda environment
    conda_python = env_dir / "bin" / "python"
    if not conda_python.exists():
        print("❌ Error: Python not found in conda environment")
        print(f"   Searched path: {conda_python}")
        return False
    
    # Check if already using conda environment Python
    current_python = Path(sys.executable).resolve()
    target_python = conda_python.resolve()
    
    if current_python != target_python:
        print("🔄 Switching to Python interpreter in conda environment...")
        print(f"   Current Python: {current_python}")
        print(f"   Target Python: {target_python}")
        
        # Restart script using conda environment Python
        import subprocess
        try:
            # Re-run current script using conda environment Python
            result = subprocess.run([str(conda_python)] + sys.argv, 
                                  cwd=os.getcwd())
            sys.exit(result.returncode)
        except Exception as e:
            print(f"❌ Failed to restart script: {e}")
            return False
    
    # Set environment variables
    conda_prefix = str(env_dir)
    os.environ['CONDA_PREFIX'] = conda_prefix
    os.environ['CONDA_DEFAULT_ENV'] = 'pycolmap_env'
    
    # Update PATH
    bin_paths = [
        str(env_dir / "bin"),
        str(miniforge_dir / "bin"),
        str(miniforge_dir / "condabin")
    ]
    
    current_path = os.environ.get('PATH', '')
    new_path = ':'.join(bin_paths + [current_path])
    os.environ['PATH'] = new_path
    
    print("✅ Activated conda environment pycolmap_env")
    print(f"   Python path: {sys.executable}")
    
    return True

# Automatically setup conda environment
if not setup_conda_environment():
    sys.exit(1)

# Try to import pycolmap
try:
    import pycolmap
    print(f"✓ Successfully loaded pycolmap version: {pycolmap.__version__}")
except ImportError as e:
    print("❌ Error: Cannot import pycolmap")
    print(f"   Detailed error: {e}")
    print("\nSolutions:")
    print("1. Check if environment is correctly installed: ./dependencies/install_colmap.sh")
    print("2. Manual test: source dependencies/miniforge3/bin/activate && conda activate pycolmap_env && python -c 'import pycolmap'")
    sys.exit(1)

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

# Use pycolmap library for headless processing, avoiding Qt and OpenGL dependencies

# Note: This function is no longer used as we now use pycolmap library
# def run_colmap_command(...): removed, now using pycolmap API

def colmap_feature_extractor(database_path: str, image_path: str,
                           camera_model: str = "PINHOLE",
                           camera_params: Optional[str] = None,
                           num_threads: int = 8) -> bool:
    """
    COLMAP feature extraction (using pycolmap)
    
    Args:
        database_path: Database path
        image_path: Image path
        camera_model: Camera model
        camera_params: Camera parameters (format: "fx,fy,cx,cy" for PINHOLE model)
        num_threads: Number of threads
        
    Returns:
        Whether successful
    """
    try:
        logger.info("Starting feature extraction...")
        
        # Set image reading options
        reader_options = pycolmap.ImageReaderOptions()
        
        if camera_params:
            # Set camera parameters (pycolmap expects string format)
            reader_options.camera_params = camera_params  # Pass string directly
            reader_options.camera_model = camera_model
            logger.info(f"Using custom camera parameters: {camera_params}")
            logger.info(f"Camera model: {camera_model}")
        else:
            logger.info("Using default feature extraction options")
        
        # Execute feature extraction - using correct parameter names
        pycolmap.extract_features(
            database_path=database_path,
            image_path=image_path,
            camera_model=camera_model,
            reader_options=reader_options
        )
        
        logger.info("✓ Feature extraction completed successfully")
        return True
        
    except Exception as e:
        logger.error(f"✗ Feature extraction failed: {e}")
        return False

def colmap_exhaustive_matcher(database_path: str,
                            num_threads: int = 8,
                            max_ratio: float = 0.8,
                            max_distance: float = 0.7,
                            cross_check: bool = True) -> bool:
    """
    COLMAP exhaustive matching (using pycolmap)
    
    Args:
        database_path: Database path
        num_threads: Number of threads
        max_ratio: Maximum ratio
        max_distance: Maximum distance
        cross_check: Whether to perform cross check
        
    Returns:
        Whether successful
    """
    try:
        logger.info("Starting feature matching...")
        logger.info("Using default matching options")
        
        # Execute exhaustive matching - using default options
        pycolmap.match_exhaustive(database_path)
        
        logger.info("✓ Feature matching completed successfully")
        return True
        
    except Exception as e:
        logger.error(f"✗ Feature matching failed: {e}")
        return False

def colmap_mapper(database_path: str, image_path: str, output_path: str,
                 num_threads: int = 8,
                 init_min_num_inliers: int = 100,
                 extract_colors: bool = True,
                 min_num_matches: int = 15) -> bool:
    """
    COLMAP incremental reconstruction (using pycolmap)
    
    Args:
        database_path: Database path
        image_path: Image path
        output_path: Output path
        num_threads: Number of threads
        init_min_num_inliers: Initial minimum number of inliers
        extract_colors: Whether to extract colors
        min_num_matches: Minimum number of matches
        
    Returns:
        Whether successful
    """
    try:
        logger.info("Starting incremental reconstruction...")
        
        # Use default options to avoid complex parameter settings
        logger.info("Using default reconstruction options")
        
        # Execute incremental reconstruction - using default options
        maps = pycolmap.incremental_mapping(
            database_path=database_path,
            image_path=image_path,
            output_path=output_path
        )
        
        if maps:
            logger.info(f"✓ Incremental reconstruction completed successfully, generated {len(maps)} reconstruction models")
            return True
        else:
            logger.warning("Incremental reconstruction completed but no valid models generated")
            return False
        
    except Exception as e:
        logger.error(f"✗ Incremental reconstruction failed: {e}")
        return False

def colmap_pipeline(image_folder: str, output_folder: str,
                   camera_model: str = "PINHOLE",
                   camera_params: Optional[str] = None,
                   num_threads: int = 8,
                   max_ratio: float = 0.8,
                   max_distance: float = 0.7,
                   init_min_num_inliers: int = 100,
                   min_num_matches: int = 15) -> bool:
    """
    Complete COLMAP incremental reconstruction pipeline
    
    Args:
        image_folder: Input image folder path
        output_folder: Output folder path
        camera_model: Camera model (PINHOLE, RADIAL, OPENCV, etc.)
        camera_params: Camera parameters (format: "fx,fy,cx,cy" for PINHOLE model)
        num_threads: Number of threads
        max_ratio: SIFT matching maximum ratio
        max_distance: SIFT matching maximum distance
        init_min_num_inliers: Initial minimum number of inliers
        min_num_matches: Minimum number of matches
        
    Returns:
        Whether reconstruction completed successfully
    """
    try:
        logger.info("="*60)
        logger.info("Starting COLMAP incremental reconstruction pipeline (using pycolmap library)")
        logger.info("="*60)
        
        # Validate input path
        image_path = Path(image_folder)
        if not image_path.exists():
            logger.error(f"Input image path does not exist: {image_path}")
            return False
        
        # Create output directory
        output_path = Path(output_folder)
        output_path.mkdir(parents=True, exist_ok=True)
        
        # Set paths
        database_path = output_path / "database.db"
        sparse_path = output_path / "sparse"
        sparse_path.mkdir(exist_ok=True)
        
        logger.info(f"Image path: {image_path}")
        logger.info(f"Output path: {output_path}")
        logger.info(f"Database path: {database_path}")
        logger.info(f"Sparse model path: {sparse_path}")
        
        # Step 1: Feature extraction
        logger.info("\n--- Step 1: Feature extraction ---")
        if not colmap_feature_extractor(
            database_path=str(database_path),
            image_path=str(image_path),
            camera_model=camera_model,
            camera_params=camera_params
        ):
            return False
        
        # Step 2: Feature matching
        logger.info("\n--- Step 2: Feature matching ---")
        if not colmap_exhaustive_matcher(
            database_path=str(database_path),
            num_threads=num_threads,
            max_ratio=max_ratio,
            max_distance=max_distance
        ):
            return False
        
        # Step 3: Incremental reconstruction
        logger.info("\n--- Step 3: Incremental reconstruction ---")
        if not colmap_mapper(
            database_path=str(database_path),
            image_path=str(image_path),
            output_path=str(sparse_path),
            num_threads=num_threads,
            init_min_num_inliers=init_min_num_inliers,
            min_num_matches=min_num_matches
        ):
            return False
        
        # Check results
        model_path = sparse_path / "0"
        if model_path.exists():
            logger.info(f"\n✓ Reconstruction successful! Model saved at: {model_path}")
            
            # Display model files
            files = ["cameras.txt", "images.txt", "points3D.txt"]
            for file in files:
                file_path = model_path / file
                if file_path.exists():
                    logger.info(f"  - {file}: ✓")
                else:
                    logger.warning(f"  - {file}: ✗")
        else:
            logger.warning("Reconstruction completed but model files not found")
        
        logger.info("="*60)
        logger.info("COLMAP incremental reconstruction pipeline completed!")
        logger.info(f"Results saved at: {output_path}")
        logger.info("="*60)
        
        return True
        
    except Exception as e:
        logger.error(f"Pipeline execution failed: {e}")
        return False

def main():
    """
    Command line entry function
    """
    import argparse
    
    parser = argparse.ArgumentParser(
        description='COLMAP incremental reconstruction pipeline (using pycolmap library)',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    
    # Required arguments
    parser.add_argument('--image_folder', required=True,
                       help='Input image folder path')
    parser.add_argument('--output_folder', required=True,
                       help='Output folder path')
    
    # Optional arguments
    parser.add_argument('--camera_model', default='PINHOLE',
                       choices=['PINHOLE', 'RADIAL', 'OPENCV', 'OPENCV_FISHEYE', 'FULL_OPENCV'],
                       help='Camera model')
    parser.add_argument('--camera_params', default=None,
                       help='Camera intrinsics (format: "fx,fy,cx,cy" for PINHOLE model)')
    parser.add_argument('--num_threads', type=int, default=8,
                       help='Number of threads')
    parser.add_argument('--max_ratio', type=float, default=0.8,
                       help='SIFT matching maximum ratio')
    parser.add_argument('--max_distance', type=float, default=0.7,
                       help='SIFT matching maximum distance')
    parser.add_argument('--init_min_num_inliers', type=int, default=100,
                       help='Initial minimum number of inliers')
    parser.add_argument('--min_num_matches', type=int, default=15,
                       help='Minimum number of matches')
    
    args = parser.parse_args()
    
    success = colmap_pipeline(
        image_folder=args.image_folder,
        output_folder=args.output_folder,
        camera_model=args.camera_model,
        camera_params=args.camera_params,
        num_threads=args.num_threads,
        max_ratio=args.max_ratio,
        max_distance=args.max_distance,
        init_min_num_inliers=args.init_min_num_inliers,
        min_num_matches=args.min_num_matches
    )
    
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()