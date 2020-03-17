import sys

def FindArgument( search ):
    if search in sys.argv:
        index = 0
        for arg in sys.argv:
            if search == sys.argv[ index ]:
                return sys.argv[ index + 1 ]
            index += 1
    else:
        return False

broken_sln_path = FindArgument( "-sln" )

broken_sln = open( broken_sln_path, "r", encoding = "utf-8" )
sln_lines = broken_sln.readlines()
broken_sln.close()

with open( broken_sln_path.rsplit( ".sln", 1 )[0] + "_fixed.sln", mode = "w", encoding = "utf-8" ) as fixed_sln:
    for line in sln_lines:
        if line.startswith( "Project(\"\")" ):
            line = "Project(\"!\")" + line.split( "Project(\"\")", 1 )[1]
        fixed_sln.write( line )

print( "done. vpc can die" )