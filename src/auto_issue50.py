#Stripped from https://github.com/jhandley/pyvcproj
import re

_REGEX_PROJECT_FILE = re.compile(r'Project\("\{([^\}]+)\}"\)[\s=]+"([^\"]+)",\s"(.+proj)", "(\{[^\}]+\})"')
_REGEX_END_PROJECT = re.compile(r"""\s*EndProject""")

class Solution(object):
    """Visual C++ solution file (.sln)."""

    def __init__(self, filename):
        """Create a Solution instance for solution file *name*."""
        self.filename = filename
        self.projects = []
        with open(self.filename, 'rb') as f:
            line = f.readline()
            while line:
                line = f.readline().decode('utf-8')
                if line.startswith("Project"):
                    match = _REGEX_PROJECT_FILE.match(line)
                    if match:
                        self.projects.append(Solution.__read_project(match.groups(), f))
                    else:
                        print('No MATCH: {0}'.format(line))

    @staticmethod
    def __read_project(project, f):
        while True:                          
            line = f.readline().decode('utf-8')
            if line is None:
                raise SolutionFileError("Missing end project")
            if _REGEX_END_PROJECT.match(line):
                break
        return project

    def project_names(self):
        """List project files (.vcxproj.) in solution."""
        return map(lambda p: p[1], self.projects)
	
from os	import getcwd
from subprocess import Popen, PIPE
		
successfulprojects = list()
failedprojects = list()		

def CheckSolution(solutionname):		
	for project in Solution(solutionname).project_names():
		devenv = Popen('\"C:/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise/Common7/IDE/devenv.com\" {}\{} /Build Release|x64 /Project {} /ProjectConfig Release|x64'.format(getcwd(), solutionname, project), shell=True, stdout=PIPE)
		while True:
			output = devenv.stdout.readline()
			if output:
				print(output.strip().decode("utf-8"))
			if devenv.poll() is not None:
				break
		if devenv.returncode == 0:
			successfulprojects.append(project)
		else:
			failedprojects.append(project)
			

CheckSolution("everything.sln")

from github import Github
import datetime
import sys

issuebody = '{}\n'.format(datetime.date.today())
for project in successfulprojects:
	issuebody += '- [x] {}\n'.format(project)
	
for project in failedprojects:
	issuebody += '- [ ] {}\n'.format(project)
	
g = Github(sys.argv[1])
	
for repo in g.get_user().get_repos():
	if repo.name == 'Engine':
		repo.get_issue(50).edit(body=issuebody)