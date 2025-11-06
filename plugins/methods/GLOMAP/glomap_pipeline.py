#!/usr/bin/env python3
# Use glomap to do the pipeline

import os
import sys
import shutil
import logging
import subprocess
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
    
    # Find possible conda installation paths
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
        print("❌ Error: Python in conda environment does not exist")
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

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def run_glomap_command(cmd_args: list, description: str = "") -> bool:
    """
    Run GLOMAP command
    
    Args:
        cmd_args: Command argument list
        description: Command description
        
    Returns:
        Whether execution was successful
    """
    try:
        if description:
            logger.info(f"Executing: {description}")
        
        logger.info(f"Command: {' '.join(cmd_args)}")
        
        result = subprocess.run(
            cmd_args,
            capture_output=True,
            text=True,
            check=False
        )
        
        if result.returncode == 0:
            logger.info(f"✓ {description} completed successfully")
            if result.stdout.strip():
                logger.debug(f"Output: {result.stdout}")
            return True
        else:
            logger.error(f"✗ {description} failed")
            logger.error(f"Error code: {result.returncode}")
            if result.stderr.strip():
                logger.error(f"Error message: {result.stderr}")
            if result.stdout.strip():
                logger.error(f"Output message: {result.stdout}")
            return False
            
    except FileNotFoundError:
        logger.error(f"✗ GLOMAP command not found, please check environment configuration")
        return False
    except Exception as e:
        logger.error(f"✗ Exception occurred while executing command: {e}")
        return False

def glomap_mapper(database_path: str, image_path: str, output_path: str, glomap_bin_path: str) -> bool:
    """
    GLOMAP Mapper - Globally optimized SfM reconstruction
    
    Args:
        database_path: Database path
        image_path: Image path
        output_path: Output path
        glomap_bin_path: glomap binary file path
        
    Returns:
        Whether successful
    """
    # Build complete glomap path
    glomap_executable = os.path.join(glomap_bin_path, "glomap")
    
    cmd = [
        glomap_executable, "mapper",
        "--database_path", database_path,
        "--image_path", image_path,
        "--output_path", output_path
    ]
    
    return run_glomap_command(cmd, "GLOMAP global optimization reconstruction")

def glomap_pipeline(database_path: str, image_path: str, output_path: str, glomap_bin_path: str) -> bool:
    """
    GLOMAP reconstruction complete pipeline
    
    Args:
        database_path: Database path (usually generated by COLMAP)
        image_path: Image path
        output_path: Output path
        glomap_bin_path: glomap binary file path
        
    Returns:
        Whether reconstruction completed successfully
    """
    try:
        logger.info("="*60)
        logger.info("Starting GLOMAP global optimization reconstruction pipeline")
        logger.info("="*60)
        
        # Validate input paths
        db_path = Path(database_path)
        if not db_path.exists():
            logger.error(f"Database file does not exist: {db_path}")
            return False
        
        img_path = Path(image_path)
        if not img_path.exists():
            logger.error(f"Image path does not exist: {img_path}")
            return False
        
        # Create output directory
        out_path = Path(output_path)
        out_path.mkdir(parents=True, exist_ok=True)
        
        logger.info(f"Database path: {db_path}")
        logger.info(f"Image path: {img_path}")
        logger.info(f"Output path: {out_path}")
        
        # Execute GLOMAP mapping
        logger.info("\n--- Executing GLOMAP global optimization reconstruction ---")
        if not glomap_mapper(
            database_path=str(db_path),
            image_path=str(img_path),
            output_path=str(out_path),
            glomap_bin_path=glomap_bin_path
        ):
            return False
        
        # Check results
        # GLOMAP usually generates model files directly in output path
        model_files = ["cameras.txt", "images.txt", "points3D.txt"]
        success_count = 0
        
        for file in model_files:
            file_path = out_path / file
            if file_path.exists():
                logger.info(f"  - {file}: ✓")
                success_count += 1
            else:
                logger.warning(f"  - {file}: ✗")
        
        if success_count > 0:
            logger.info(f"\n✓ Reconstruction successful! Model saved at: {out_path}")
            logger.info(f"  Generated files: {success_count}/{len(model_files)}")
        else:
            logger.warning("Reconstruction completed but expected model files not found")
        
        logger.info("="*60)
        logger.info("GLOMAP global optimization reconstruction pipeline completed!")
        logger.info(f"Results saved at: {out_path}")
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
        description='GLOMAP global optimization reconstruction pipeline',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    
    # Required arguments
    parser.add_argument('--database_path', required=True,
                       help='Database file path (usually database.db generated by COLMAP)')
    parser.add_argument('--image_path', required=True,
                       help='Image folder path')
    parser.add_argument('--output_path', required=True,
                       help='Output folder path')
    parser.add_argument('--glomap_bin_path', required=True,
                       help='GLOMAP binary file directory path')
    
    args = parser.parse_args()
    
    success = glomap_pipeline(
        database_path=args.database_path,
        image_path=args.image_path,
        output_path=args.output_path,
        glomap_bin_path=args.glomap_bin_path
    )
    
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main() 