# Creates projects 
import subprocess, platform, sys, os, xml, threading, platform
import xml.etree.ElementTree as ET

# VPC environment vars can be any of the following
env_vars = [
	"QUIVERVPC",
	"VPC",
	"VPCEXE",
]

# Global lock
stdlock = threading.Lock()

class VPCCommand():

	def __init__(self, plat, param, nam):
		self.platform = plat
		self.params = param
		self.name = nam

	# Platform
	platform = ""

	# Actual params
	params = ""

	# Conf name
	name = ""

class ProjectGenerator():

	# List of commands
	commands = list()

	# Vpc command
	vpccmd = ""

	# List of platforms to generate for
	platforms = list()

	@staticmethod
	def GenerateProjects(plats):
		inst = ProjectGenerator()

		# Take care of null case
		if plats == None:
			if os.name == "posix":
				inst.platforms = ["posix"]
			elif "Windows" in platform.system() or "windows" in platform.system():
				inst.platforms = ["windows"]
		else:
			inst.platforms = plats
		
		inst.CheckVariables()
		inst.ParseXML()
		inst.RunThreads()

	def ParseXML(self):
		try:
			doc = ET.parse('vpc_scripts/commands.xml')
			root = doc.getroot()
			for i in root.getiterator():
				# Look through all elements yay
				if i.tag == "command":
					if i.get("name") == None:
						printf("ERROR: Command without name attribute. Skipping...")
					elif i.get("platform") != None:
						self.commands.append(VPCCommand(i.get("platform"), i.text, i.get("name")))
					else:
						print("WARNING: Config " + i.get("name") + " has no specified platform, this command will generate for all platforms.")
						self.commands.append(VPCCommand("any", i.text, i.get("name")))
		except FileNotFoundError as e:
			print("The file vpc_scripts/commands.xml does not exist!")
			exit(1)
		except:
			print("Error while parsing commands.xml.")
			exit(1)
	
	def CheckVariables(self):
		for i in env_vars:
			var = os.getenv(str(i))
			if var != None:
				self.vpccmd = var
				return None
		# oopsie vars dont exist, warn then try to see if another vpc dir can be found
		print("WARNING: vpc executable not found in env var.\nPlease set the environment var 'QUIVERVPC' to the vpc exe\nSetting default..")
		if(os.name == "posix"):
			self.vpccmd = "devtools/bin/vpc_linux"
		else:
			self.vpccmd = "devtools/bin/vpc.exe"
		
		# See if it exists
		try:
			subprocess.run(self.vpccmd)
		except:
			print("Unable to find vpc.")
			exit(1)

	def RunThreads(self):
		# Make current command
		for i in self.commands:
			if i.platform in self.platforms:
				cmd = self.vpccmd + " "
				for st in i.params:
					cmd += st
				# Quick fix up for windows. If we generate for windows, use mksln and set it to name of command
				if "windows" in i.platform or "Windows" in i.platform:
					cmd += " " + "/mksln " + i.name
				threading.Thread(target=ThreadInstance.threadproc(cmd)).run()

class ThreadInstance():
	@staticmethod
	def threadproc(cmd):
		try:
			# Run
			ret = subprocess.run([cmd], stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)

			# get lock so we dont spew at the same time
			stdlock.acquire()
			try:
				# fix up and print stdout
				print(str(ret.stdout).replace('\\n', '\n'))
			finally:
				stdlock.release()
			
		except Exception as e:
			print("Internal error")
			print(e)

# Runs our program
def main():
	bFoundPlat = False
	plats = list()
	for i in sys.argv:
		if "--help" in i or "-h" in i:
			print("---------------------- Params ----------------------")
			print("-a            - Generates projects for all platforms")
			print("-p <platform> - Generate projects for the specified platform")
			exit(0)
		elif i == "-p":
			bFoundPlat = True
		elif bFoundPlat == True:
			plats.append(i)
			bFoundPlat = False
	if bFoundPlat == True:
		print("Invalid platform.")
		exit(1)

	print("Generating projects...")
	ProjectGenerator.GenerateProjects(plats)
	print("\n\nDone generating!")

main()