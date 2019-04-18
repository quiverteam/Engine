#
#
# Makefile parser for vscode
#
#
import os, io, json, sys, pathlib



	

#
# Converts the specified makefile in str in the dir to a vscode boi
#
def ConvertFile(file: str, dir: str):
	defines = None
	includes = None
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
	# Now create vscode dir inside the specified dir
	if not pathlib.Path(dir + "/.vscode/").exists():
		os.mkdir(dir + "/.vscode/")
	dir += "/.vscode/"
	# Create json doc

	doc = '{\n"configurations": [\n'
	doc += '{\n'
	doc += '"name": "Debug",\n'
	doc += '"includePath": [\n'
	
	if includes == None:
		includes = []

	for include in includes:
		#include.replace('../', '')
		doc += '"' + str(pathlib.Path(include).resolve()) + '",\n'
	
	# Stupid commas fuck json
	if len(includes) > 1:
		doc += '"' + includes[0] + '"\n'
	
	doc += '\n],\n"defines":['

	if defines == None:
		defines = []

	for define in defines:
		doc += '"' + define + '",\n'

	# Stupid commas fuck json
	if len(defines) > 1:
		doc += '"' + defines[0] + '"\n'

	doc += '],\n'
	doc += '"compilerPath": "/usr/bin/gcc",\n'
	doc += '"cStandard": "c11",\n'
	doc += '"cppStandard": "c++11",\n'
	doc += '"intelliSenseMode": "gcc-x64"\n'
	doc += '}\n'
	doc += '],\n'
	doc += '"version": 4\n'
	doc += '}\n'

	print("Writing Linter Config...")
	with io.open(dir + "c_cpp_properties.json", "w+") as stream:
		for c in doc:
			stream.write(c)

	filename = os.path.basename(file)
	workspace = str(pathlib.Path(dir + "/../" + filename + ".code-workspace").resolve())
	print("Writing Workspace file: " + workspace)

	doc = ""
	doc += '{\n"folders":[\n'
	for include in includes:
		doc += '{\n'
		doc += '"path": "' + str(pathlib.Path(include).resolve()) + '"'
		doc += '},\n'
	
	doc += '{\n"path": "."\n}\n'
	doc += '],\n"settings": {}\n'
	doc += '}\n'

	with io.open(workspace, "w+") as stream:
		for c in doc:
			stream.write(c)
	
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
		macro.replace("-D", "")
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
	
	path = False
	apath = ''
	for param in sys.argv:
		if path == True:
			apath = param

		if param == '-p':
			path = True

	if apath != '':
		ConvertDir(apath)
main()