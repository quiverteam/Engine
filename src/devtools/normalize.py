#!/usr/bin/env python3

# Script to normalize include case and file names
import sys, os, re

def show_help():
	print("USAGE: normalize.py [options] folder")
	exit(0)

folder = None
recursive = True 
depth = 0
files = list()
pattern = re.compile(r"(?<=\"|<)[^\"<]+(?=\"|>)")

def main():
	for arg in sys.argv:
		if arg == "--help" or arg == "-h":
			show_help()
		if arg == "--recursive":
			global recursive
			recursive = True 
	global folder
	folder = sys.argv[len(sys.argv)-1]
	normalize_folder(folder)

def normalize_folder(folder):
	print("--- Entering directory " + folder + " ---")
	for f in os.listdir(folder):
		if os.path.isdir(f) and recursive:
			normalize_folder(f)
		if not os.path.isfile(folder + f):
			continue

		# Check if we've got a C++ or C source
		if f.endswith(".cpp") or f.endswith(".c") or f.endswith(".cc") or f.endswith(".cxx") or f.endswith(".h") or f.endswith(".hpp") or f.endswith(".hxx"):
			os.rename(folder + f, folder + f.lower())
			print(str(f) + " -> " + f.lower())
			files.append(f)

def normalize_includes(folder):
	print("--- Normalizing includes " + folder + " ---")
	for f in os.listdir(folder):
		if os.path.isdir(f) and recursive:
			normalize_includes(folder)
		if not os.path.isfile(f):
			continue

		# Check if we've got a C++ or C source
		if f.endswith(".cpp") or f.endswith(".c") or f.endswith(".cc") or f.endswith(".cxx") or f.endswith(".h") or f.endswith(".hpp") or f.endswith(".hxx"):
			with open(folder + "/" + f, "rw") as fs:
				print("FUCK")
				lines = fs.readlines()
				for i in range(len(lines)):
					if lines[i].startswith("#include"):
						match = pattern.match(lines[i])
						if match != None:
							lines[i].replace(str(match), str(match).lower())
				fs.seek(0)
				fs.writelines(lines)
		print("Fixed " + f)

main()