#!/usr/bin/env python3
# File: dist2loc.py

import argparse
import os
import shutil
import sys

# --- Configuration ---
# You can define your root directories here. Use a full, absolute path.

# Define the root directory for your game server's destination files.
# For example: '/usr/home/game'
GAME_ROOT_DIR = "/usr/home/game"

# Define the root directory where your source code and build output are located.
# For example: '/usr/home/src'
SOURCE_ROOT_DIR = "/usr/home/src"

# --- Derived Paths (Do not change below this line) ---
# Source directory for compiled binaries
SOURCE_DIR = os.path.join(SOURCE_ROOT_DIR, 'build', 'bin')

# Destination directories are based on the GAME_ROOT_DIR
GAME_DEST_DIR = os.path.join(GAME_ROOT_DIR, 'share', 'bin')
DB_DEST_DIR = os.path.join(GAME_ROOT_DIR, 'share', 'bin')
LOCALE_DIR = os.path.join(GAME_ROOT_DIR, 'share', 'locale')

# --- Argument Parsing ---
def parse_arguments():
	"""
	Parses command-line arguments to determine which files to copy.
	"""
	parser = argparse.ArgumentParser(
		description="Automate the deployment of game, db, and qc binaries.",
		epilog="Example: python3 deploy_game_binaries.py --all"
	)
	
	# Mutually exclusive group for the flags
	group = parser.add_mutually_exclusive_group()
	group.add_argument(
		'--all',
		action='store_true',
		help="Transfer all three files: game, db, and qc."
	)
	group.add_argument(
		'--files',
		nargs='+',
		choices=['game', 'db', 'qc'],
		help="Specify which files to transfer (e.g., game db qc)."
	)

	args = parser.parse_args()

	# If --all is used, set all individual files to be transferred.
	if args.all:
		args.files = ['game', 'db', 'qc']
	elif args.files is None:
		parser.print_help()
		sys.exit(1)

	return args

# --- Core Functions ---
def set_permissions(path: str, permission_code: int) -> None:
	"""
	Sets the specified permission code on a given file.
	"""
	if os.name == "nt":
		pass
	else:
		try:
			os.chmod(path, permission_code)
			print(f"  - Set permissions to {oct(permission_code)[2:]} on: {path}")
		except OSError as e:
			print(f"  - ERROR: Failed to set permissions on {path}: {e}")

def find_qc_destinations():
	"""
	Searches all locale subdirectories for a 'quest' folder that contains
	a 'questlib.lua' file.
	Returns a list of all valid quest folder paths.
	"""
	destinations = []
	# Make sure LOCALE_DIR exists before walking it
	if not os.path.isdir(LOCALE_DIR):
		print(f"Warning: Locale directory not found at '{LOCALE_DIR}'. No 'qc' destinations will be found.")
		return destinations
	for root, dirs, files in os.walk(LOCALE_DIR):
		# Check if the current directory is named 'quest'
		if os.path.basename(root) == 'quest':
			# Check if questlib.lua exists in this 'quest' folder
			if 'questlib.lua' in files:
				destinations.append(root)
	return destinations

def copy_file(source, destination):
	"""
	Copies a single file from source to destination, handling errors.
	Appends '.exe' to paths if running on Windows (os.name == 'nt').
	"""
	
	# Check if the environment is Windows
	if os.name == "nt":
		# Conditionally append .exe to destination if it's not already there
		if not source.lower().endswith(".exe"):
			source = source + ".exe"
	
	# Execute the copy operation
	try:
		shutil.copy2(source, destination)
		print(f"  - Copied '{os.path.basename(source)}' to '{destination}'")
		return True
	
	except FileNotFoundError:
		print(f"  - ERROR: File not found at '{source}'. Skipping.")
	except Exception as e:
		print(f"  - ERROR: Failed to copy '{os.path.basename(source)}'. Reason: {e}")
		
	return False

def main():
	"""
	Main function to orchestrate the deployment process.
	"""
	args = parse_arguments()

	print("\nStarting game binary deployment...")
	print(f"Source directory: {SOURCE_DIR}\n")

	# Mapping of file names to their source and destination paths
	file_map = {
		'game': (os.path.join(SOURCE_DIR, 'game'), GAME_DEST_DIR),
		'db': (os.path.join(SOURCE_DIR, 'db'), DB_DEST_DIR),
		'qc': (os.path.join(SOURCE_DIR, 'qc'), None) # Destination for 'qc' is dynamic
	}
	
	# Files to set permissions on after they are copied
	permissions_targets = []

	# Handle 'qc' separately to copy to multiple destinations
	if 'qc' in args.files:
		qc_dest_dirs = find_qc_destinations()
		if not qc_dest_dirs:
			print("ERROR: No 'quest' folders containing 'questlib.lua' found in any locale. Skipping 'qc' transfer.")
		else:
			print("Processing 'qc'...")
			for dest_dir in qc_dest_dirs:
				dest_file = os.path.join(dest_dir, os.path.basename(file_map['qc'][0]))
				if copy_file(file_map['qc'][0], dest_dir):
					permissions_targets.append(dest_file)
		args.files.remove('qc') # Remove to avoid processing again

	# Perform the copy operations for the remaining files
	for file_to_copy in args.files:
		source_path, dest_path = file_map.get(file_to_copy)
		dest_file = os.path.join(dest_path, os.path.basename(source_path))
		
		print(f"Processing '{file_to_copy}'...")
		if copy_file(source_path, dest_path):
			permissions_targets.append(dest_file)

	# Only attempt to set permissions on Unix-like systems, not Windows (os.name == "nt")
	if os.name != "nt":
		# 777 in octal
		permission_code = 0o777
		
		print("\nSetting permissions...")
		for target in permissions_targets:
			set_permissions(target, permission_code)

	print("\nDeployment and permissions complete.")

if __name__ == "__main__":
	main()