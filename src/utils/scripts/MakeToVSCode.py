#
#
# Makefile parser for vscode
#
#
import os, io, json, sys, pathlib, glob
from copy import deepcopy


Configurations = {
	"configurations": [
		{
			"name": "Debug",
			"includePath": [
				
			],
			"defines": [
				
			],
			"compilerPath": "/usr/bin/gcc",
			"cStandard": "c11",
			"cppStandard": "c++11",
			"intelliSenseMode": "gcc-x64",
		}
	],
	"version": 4
}

Workspace = {
	"folders": [
	]
}

#
# Converts the specified makefile in str in the dir to a vscode boi
# This is pretty dumb actually. There are no makefile parsers for python unfortunately
#
def ConvertFile(file: str, dir: str):
	defines = None
	includes = None
	
	#
	# Read makefile
	#
	print("Reading...")
	with io.open(file, "r") as stream:
		line = stream.readline()
		while line != "" and (defines == None or includes == None):
			if "DEFINES" in line:
				line = line.replace("DEFINES", "")
				line = line.replace("+=", "")
				defines = ParseMacros(line.split(None))
			if "INCLUDEDIRS" in line:
				line = line.replace("INCLUDEDIRS", "")
				line = line.replace("+=", "")
				includes = line.split(None)
			line = stream.readline()
	
	#
	# Create the linter config
	# Now create vscode dir inside the specified dir
	if not pathlib.Path(dir + "/.vscode/").exists():
		os.mkdir(dir + "/.vscode/")
	dir += "/.vscode/"
	# Create json doc

	doc = deepcopy(Configurations)
	
	if includes == None:
		includes = []

	for include in includes:
		doc["configurations"][0]["includePath"].append(str(include))

	if defines == None:
		defines = []

	doc["configurations"][0]["defines"].append("VPROF_LEVEL")# "GNUC", "NO_HOOK_MALLOC", "NO_MALLOC_OVERRIDE")
	
	for define in defines:
		doc["configurations"][0]["defines"].append(str(define))

	print("Writing Linter Config...")
	with io.open(dir + "c_cpp_properties.json", "w+") as stream:
		json.dump(doc, stream, indent=4)

	#
	# Create workspace for vscode
	#
	filename = os.path.basename(file)
	path = pathlib.Path(dir + "/../" + filename).with_suffix('.code-workspace').resolve()
	workspace = str(path)
	print("Writing Workspace file: " + workspace)

	doc = deepcopy(Workspace)
	
	for include in includes:
		doc["folders"].append({"path": str(include)})

	with io.open(workspace, "w+") as stream:
		json.dump(doc, stream, indent=4)
	
	print("Done!")

	

			


def ConvertDir(dir: str):
	print("========= Scanning " + str(pathlib.Path(dir).resolve()) + " =========")
	for d in os.listdir(dir):
		if str(d).endswith(".mak"):
			print("Found makefile: " + str(d))
			print("Converting...")
			ConvertFile(dir + "/" + d, str(pathlib.Path(dir).resolve()))
		if os.path.isdir(dir + "/" + d):
			ConvertDir(dir + "/" + d)

def ParseMacros(macros: list) -> list:
	ret = list()
	for macro in macros:
		macro = macro.replace("-D", "")
		tokens = macro.split("=")
		ret.append(tokens[0])
	return ret
			
def ParseIncludes(includes: list) -> list:
	ret = list()
	for include in includes:
		pathlib.Path()

#
# Parses makefiles and creates vscode projects from them
# dirs is a list of dirs to look for makefiles in
# this will generate a .vscode directory inside of the file
#

def MakeToVSCode(dirs: list):
	for d in dirs:
		ConvertDir(d)

def main():
	if "-h" in sys.argv or "--help" in sys.argv:
		print("========= Help =========")
		print('-h --help		Prints this help')
		print('-p <path>		Base directory')
		print('-c 				Cleans all workspace definitions')
	
	path = False
	apath = ''
	for param in sys.argv:
		if path == True:
			apath = param
			path = False	

		if param == '-p':
			path = True
			
		if param == '-c':
			print("Cleaning")
			gl = glob.glob('./*/*.code-workspace', recursive=True)
			for f in gl:
				print("Removing " + str(f) + ".")
				os.remove(f)

	if apath != '':
		ConvertDir(apath)
main()
