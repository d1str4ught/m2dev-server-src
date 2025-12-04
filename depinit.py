import os
import subprocess
import sys
import shutil
from pathlib import Path
import time
import stat

# Use this error handler for shutil.rmtree on Windows (required for robust cleanup)
def handle_remove_readonly(func, path, exc_info):
    """If the file is readonly, change its permission and retry."""
    if func in (os.rmdir, os.remove, os.unlink) and exc_info[1].winerror == 5:
        os.chmod(path, stat.S_IWUSR | stat.S_IREAD)
        func(path)
    else:
        # Re-raise the exception if it's not a permission issue
        raise

# --- Configuration ---
# All paths are relative to the project root.
PROJECT_ROOT = Path(__file__).parent
INCLUDE_DIR = PROJECT_ROOT / "include"

# Define all dependencies, their Git URLs, and which files need copying.
# NOTE: Removed cryptopp/src and added top-level cryptopp as extraction
DEPS_INFO = [
    # Full Submodules (Clone and leave intact)
    {"name": "spdlog", "type": "submodule", "path": "vendor/spdlog", "url": "https://github.com/gabime/spdlog.git", "copy": []},
    {"name": "mariadb-connector-c", "type": "submodule", "path": "vendor/mariadb-connector-c", "url": "https://github.com/MariaDB/mariadb-connector-c.git", "copy": []},
    
    # Library Extraction (Custom Source Copy)
    # NOTE: Assuming you want to replace the old CMake submodule update with the new client-style extraction
    {"name": "cryptopp", "type": "extract", "path": "vendor/cryptopp", "url": "https://github.com/weidai11/cryptopp.git", "copy": []},

    # Header-Only Extraction (Temporary Clone, Copy Headers, Clean up vendor directory)
    {"name": "stb", "type": "extract", "path": "vendor/stb", "url": "https://github.com/nothings/stb.git", "copy": [("stb_image.h", "."), ("stb_image_write.h", ".")]},
    {"name": "pcg-cpp", "type": "extract", "path": "vendor/pcg-cpp", "url": "https://github.com/imneme/pcg-cpp.git", "copy": [("include/pcg_random.hpp", "."), ("include/pcg_extras.hpp", "."), ("include/pcg_uint128.hpp", ".")]},
]

# --- Utility Functions ---

def run_git_command(command, cwd=PROJECT_ROOT):
	"""Executes a git command and handles errors."""
	print(f" -> Executing: {' '.join(command)}")
	try:
		# check=True will raise CalledProcessError on failure
		subprocess.run(
			command,
			check=True,
			cwd=cwd,
			stdout=sys.stdout,
			stderr=sys.stderr,
			text=True
		)
		return True
	except subprocess.CalledProcessError as e:
		print(f" ERROR: Git command failed: {' '.join(command)}")
		print(f" Stderr: {e.stderr}")
		raise # Propagate the error to halt the script
	except FileNotFoundError:
		print(" FATAL ERROR: 'git' command not found. Ensure Git is installed and in your PATH.")
		sys.exit(1)


def handle_submodule(dep):
	"""Initializes and updates a dependency as a standard Git submodule."""
	target_path = PROJECT_ROOT / dep['path']
	repo_url = dep['url']

	# 1. Check if the directory exists. If not, run 'submodule add'
	if not target_path.exists():
		print(f" Submodule path not found: {dep['path']}")
		print(" -> Attempting 'git submodule add' to fix the repository index.")
		# NOTE: Using --force to overwrite if path exists but is not tracked
		command = ["git", "submodule", "add", "--force", repo_url, dep['path']]
		run_git_command(command)
	
	# 2. Update the submodule content
	print(f" -> Updating submodule content: {dep['name']}")
	command = ["git", "submodule", "update", "--init", "--recursive", dep['path']]
	run_git_command(command)


def handle_extraction(dep):
	"""Clones, extracts specific files/folders, and cleans up the temporary clone."""
	name = dep["name"]
	repo_url = dep["url"]
	target_dir = PROJECT_ROOT / dep["path"]
	tmp_path = PROJECT_ROOT / f".tmp_{name.replace('/', '_')}"

	print(f" -> Managing extraction dependency: {name}...")

	# 1. Pre-cleanup of temporary directory
	if tmp_path.exists():
		print(f" Pre-cleanup: Deleting stale temporary directory: {tmp_path}")
		shutil.rmtree(tmp_path, onerror=handle_remove_readonly)
	
	# 2. Clone into a temporary directory
	print(f" Cloning into temporary directory: {tmp_path}")
	clone_command = ["git", "clone", "--depth", "1", repo_url, str(tmp_path)]
	run_git_command(clone_command)

	# --- 3. Handle CryptoPP Custom Extraction (WITH CMakeLists.txt Protection) ---
	if name == "cryptopp":
		cmakelists_file = target_dir / "CMakeLists.txt"
		backup_path = PROJECT_ROOT / ".tmp_cmakelists_cryptopp_server" # Use a unique temp name

		# 3a. BACKUP: Move your custom CMakeLists.txt to a safe location
		if cmakelists_file.exists():
			print(" -> [BK] Backing up custom CMakeLists.txt...")
			try:
				shutil.move(cmakelists_file, backup_path)
			except Exception as e:
				print(f" -> ERROR during CMakeLists.txt backup: {e}")
				raise

		# 3b. AGGRESSIVE CLEANUP: Delete the entire vendor/cryptopp directory
		print(" -> Aggressively cleaning up old CryptoPP directory...")
		if target_dir.exists():
			shutil.rmtree(target_dir, onerror=handle_remove_readonly)
		
		# 3c. RESTORE & COPY: Recreate the directory and copy content
		target_dir.mkdir(parents=True, exist_ok=True)
		
		print(" -> Copying ALL contents from temp repo to target dir.")
		for item in tmp_path.iterdir():
			if item.name.startswith('.'):
				continue
			
			destination = target_dir / item.name
			
			if item.is_dir():
				shutil.copytree(item, destination, dirs_exist_ok=True)
			elif item.is_file():
				shutil.copy2(item, destination)

		# 3d. RESTORE: Move the custom CMakeLists.txt back
		if backup_path.exists():
			print(" ->  restoring custom CMakeLists.txt...")
			shutil.move(backup_path, cmakelists_file)

	# 4. Handle Header-Only Extraction (STB, PCG-CPP)
	elif dep['copy']:
		# a. Clean up old destination (the vendor/stb or vendor/pcg-cpp directory)
		if target_dir.exists():
			shutil.rmtree(target_dir, onerror=handle_remove_readonly)
        
		# b. Ensure the final INCLUDE directory exists
		INCLUDE_DIR.mkdir(parents=True, exist_ok=True)

		print(f" -> Copying headers for {name} to include/...")
		
		# c. Copy files
		for src_in_repo, dest_in_target in dep["copy"]:
			source = tmp_path / src_in_repo
			# The destination is always just the filename in the main include directory
			destination = INCLUDE_DIR / source.name

			if source.exists():
				shutil.copy2(source, destination)
				print(f"   - Copied: {source.name}")
			else:
				print(f" WARNING: Source file not found in temporary repo: {source}")

	# 5. Final cleanup of the temporary directory
	print(f" -> Cleaning up temporary directory: {tmp_path}")
	shutil.rmtree(tmp_path, onerror=handle_remove_readonly)


def main():
	"""Main execution logic."""
	print("--- Dependency Initialization Script Running ---")
	
	# 1. Ensure global submodule sync/update for existing definitions
	run_git_command(["git", "submodule", "sync"])
	run_git_command(["git", "submodule", "update", "--init", "--recursive"])
	
	for dep in DEPS_INFO:
		print(f"\n[Processing {dep['name']}]")
		
		try:
			if dep["type"] == "submodule":
				handle_submodule(dep)
			elif dep["type"] == "extract":
				handle_extraction(dep)
			else:
				print(f"[WARNING] Unknown dependency type for {dep['name']}: {dep['type']}")
				
		except Exception as e:
			# Use the clean, structured exception reporting
			print(f"\n[ERROR] FATAL ERROR processing {dep['name']}. Aborting script.")
			print(e)
			return 1

	print("\n[DONE] All dependencies successfully checked and prepared.")
	return 0

if __name__ == "__main__":
	sys.exit(main())