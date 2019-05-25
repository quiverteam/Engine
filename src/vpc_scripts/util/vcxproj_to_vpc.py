import sys, os
import xml.etree.ElementTree as ET

# maybe add an argument for the source directory?

# argument parser
def FindArgument( search, return_value ):
    if search in sys.argv:
        index = 0
        for arg in sys.argv:
            if search == sys.argv[ index ]:
                if return_value:
                    return sys.argv[ index + 1 ]
                else:
                    return True
            index += 1
    else:
        return False


# -------------------------------------------------------------------------------------------------------------
# TODO:
# need to add checks for $(SolutionDir), $(Platform), $(Configuration), %(AdditionalIncludeDirectories)
# add PropertyGroup: <TargetName>client</TargetName> to dictionaries below
# -------------------------------------------------------------------------------------------------------------

# Dictionaries and Lists
# I HATE HOW MUCH SPACE THESE TAKE
# but it's needed, oh well

# options in the $Configuration Section with only one value
# the category name is what vpc uses (General, Linker, etc.)
# left is name in vcxproj
# right is name in vpc

# TODO: move AdditionalDependencies into link libraries folder with $Lib
ConfigOptions_SingleValue = {

    "General"   :
    {
	    "OutputDirectory"		        	:	"OutputDirectory",
	    "IntermediateDirectory"		       	:	"IntermediateDirectory",
	    "DeleteExtensionsOnClean"	    	:	"ExtensionsToDeleteOnClean",
	    "BuildLogFile"				        :	"BuildLogFile",
	    "ATLMinimizesCRunTimeLibraryUsage"	:	"MinimizeCRTUseInATL",
	    "ManagedExtensions"			        :	"UseManagedExtensions",
	    "WholeProgramOptimization"     		:	"WholeProgramOptimization",
	    "ReferencesPath"			        :	"ReferencesPath",
	    "ExcludedFromBuild"			        :	"ExcludedFromBuild",

        # pretty much mostly useless
        #"OutDir"                            :   "OutputDirectory",
    },

    "Compiler" :
	{
        "AdditionalIncludeDirectories"		:	"AdditionalIncludeDirectories",
        "AdditionalUsingDirectories"		:	"ResolveUsingReferences",
        "PreprocessorDefinitions"		    :	"PreprocessorDefinitions",

        # Precompiled Headers
	    #"PrecompiledHeaderThrough"		:	"Create/UsePCHThroughFile",
	    "PrecompiledHeader"		        :	"Create/UsePCHThroughFile",
	    "PrecompiledHeaderFile"			:	"PrecompiledHeaderFile",

	    # Output Files
	    "AssemblerListingLocation"		:	"ASMListLocation",
	    "ObjectFile"				    :	"ObjectFileName",
	    "ProgramDataBaseFileName"		:	"ProgramDatabaseFileName",
	    "XMLDocumentationFileName"		:	"XMLDocumentationFileName",

	    # Browse Information
	    "BrowseInformationFile"			:	"BrowseFile",

	    # Advanced
	    "DisableSpecificWarnings"		:	"DisableSpecificWarnings",
	    "ForcedIncludeFiles"			:	"ForceIncludes",
	    "ForcedUsingFiles"			:	"ForceUsing",
	    "UndefinePreprocessorDefinitions"	:	"UndefinePreprocessorDefinitions",

	    # Command Line (this appears in multiple files, so this will selected everytime probably, need to fix that)
	    "AdditionalOptions"			:	"AdditionalOptions",
    },

	# Librarian
	"Librarian"	:
	{
		# General
		"OutputFile"				:	"OutputFileexplo", # what
		"AdditionalDependencies"		:	"AdditionalDependencies",
		"AdditionalLibraryDirectories"		:	"AdditionalLibraryDirectories",
		"ModuleDefinitionFile"			:	"ModuleDefinitionFileName",
		"IgnoreDefaultLibraryNames"		:	"IgnoreSpecificLibrary",
		"ExportNamedFunctions"			:	"ExportNamedFunctions",
		"ForceSymbolReferences"			:	"ForceSymbolReferences",
	},

	# Linker 
	"Linker"			:
	{
		# General
		"OutputFile"				:	"OutputFile",
		"Version"				:	"Version",
		"AdditionalLibraryDirectories"		:	"AdditionalLibraryDirectories",

		# Input
		"AdditionalDependencies"		:	"AdditionalDependencies",
		"IgnoreDefaultLibraryNames"		:	"IgnoreSpecificLibrary",
		"ModuleDefinitionFile"			:	"ModuleDefinitionFile",
		"AddModuleNamesToAssembly"		:	"AddModuleToAssembly",
		"EmbedManagedResourceFile"		:	"EmbedManagedResourceFile",
		"ForceSymbolReferences"			:	"ForceSymbolReferences",
		"DelayLoadDLLs"				:	"DelayLoadedDLLs",
		"AssemblyLinkResource"			:	"AssemblyLinkResource",

		# Manifest File
		"ManifestFile"				:	"ManifestFile",
		"AdditionalManifestDependencies"	:	"AdditionalManifestDependencies",

		# Debugging
		"ProgramDatabaseFile"			:	"GenerateProgramDatabaseFile",
		"StripPrivateSymbols"			:	"StripPrivateSymbols",
		"MapFileName"				:	"MapFileName",
	
		# System
		"HeapReserveSize"			:	"HeapReserverSize",
		"HeapCommitSize"			:	"HeapCommitSize",
		"StackReserveSize"			:	"StackReserveSize",
		"StackCommitSize"			:	"StackCommitSize",

		# Optimization
		"FunctionOrder"				:	"FunctionOrder",
		"ProfileGuidedDatabase"			:	"ProfileGuidedDatabase",

		# Embedded IDL
		"MidlCommandFile"			:	"MIDLCommands",
		"MergedIDLBaseFileName"			:	"MergedIDLBaseFileName",
		"TypeLibraryFile"			:	"TypeLibrary",
		"TypeLibraryResourceID"			:	"TypeLibResourceID",

		# Advanced
		"EntryPointSymbol"			:	"EntryPoint",
		"BaseAddress"				:	"BaseAddress",
		"ImportLibrary"				:	"ImportLibrary",
		"MergeSections"				:	"MergeSections",
		"KeyFile"				:	"KeyFile",
		"KeyContainer"				:	"KeyContainer",
	},

	#"ResourceOutputFileName"		    :	"ResourceFileName",

    # -------------------------------------------------------------
    # TODO: fix the command value, need special settings for that one

	# Build Events
	"PreBuildEvent"		:
	{
		# Pre-Build Event
		#"Command"			    	:	"CommandLine",
		"Description"				:	"Description",
		"ExcludedFromBuild"			:	"ExcludedFromBuild",
	},

	"PreLinkEvent"		:
	{
		# Pre-Link Event
		#"Command"			    	:	"CommandLine",
		"Description"				:	"Description",
		"ExcludedFromBuild"			:	"ExcludedFromBuild",
	},

	"PostBuildEvent"		:
	{
		# Post-Build Event
		#"Command"			    	:	"CommandLine",
		"Description"				:	"Description",
		"ExcludedFromBuild"			:	"ExcludedFromBuild",
	},

	# Custom Build Step
	"CustomBuildStep"		:
	{
		# Pre-Build Event
		#"Command"			    	:	"CommandLine",
		"Description"				:	"Description",
		#"Outputs"				    :	"Outputs",
		"AdditionalDependencies"		:	"AdditionalDependencies",
	},
}

ConfigOptions_MutliValue = {
    
    "General"   :
    {
		"ConfigurationType"			:	"ConfigurationType",
		"UseOfMFC"				:	"UseOfMFC",
		"UseOfAtl"				:	"UseOfATL",
		"CharacterSet"				:	"CharacterSet",
    },

    "Compiler"   :
    {
		"DebugInformationFormat"		:	"DebugInformationFormat",
		"WarningLevel"				    :	"WarningLevel",
        
		"SuppressStartupBanner"			:	"SuppressStartupBanner",
		"Detect64BitPortabilityProblems"	:	"Detect64BitPortabilityIssues",
		"WarnAsError"				:	"TreatWarningsAsErrors",
		"UseUnicodeResponseFiles"		:	"UseUNICODEResponseFiles",

		# Optimization
		"Optimization"				:	"Optimization",
		"InlineFunctionExpansion"		:	"InlineFunctionExpansion",
		"EnableIntrinsicFunctions"		:	"EnableIntrinsicFunctions",
		"FavorSizeOrSpeed"			:	"FavorSizeOrSpeed",
		"OmitFramePointers"			:	"OmitFramePointers",
		"EnableFiberSafeOptimizations"		:	"EnableFiberSafeOptimizations",
		#"WholeProgramOptimization"		:	"WholeProgramOptimization",
		
		# Preprocessor
		"IgnoreStandardIncludePath"		:	"IgnoreStandardIncludePath",
		"GeneratePreprocessedFile"		:	"GeneratePreprocessedFile",
		"KeepComments"				:	"KeepComments",

		# Code Generation
		"StringPooling"				:	"EnableStringPooling",
		"MinimalRebuild"			:	"EnableMinimalRebuild",
		"ExceptionHandling"			:	"EnableC++Exceptions",
		"SmallerTypeCheck"			:	"SmallerTypeCheck",
		"BasicRuntimeChecks"			:	"BasicRuntimeChecks",
		"RuntimeLibrary"			:	"RuntimeLibrary",
		"StructMemberAlignment"			:	"StructMemberAlignement",
		"BufferSecurityCheck"			:	"BufferSecurityCheck",
		"EnableFunctionLevelLinking"		:	"EnableFunctionLevelLinking",
		"EnableEnhancedInstructionSet"		:	"EnableEnhancedInstructionSet",
		"FloatingPointModel"			:	"FloatingPointModel",
		"FloatingPointExceptions"		:	"EnableFloatingPointExceptions",

		# Language
		"DisableLanguageExtensions"		:	"DisableLanguageExtensions",
		"DefaultCharIsUnsigned"			:	"DefaultCharUnsigned",
		"TreatWChar_tAsBuiltInType"		:	"TreatWchar_tAsBuiltinType",
		"ForceConformanceInForLoopScope"	:	"ForceConformanceInForLoopScope",
		"RuntimeTypeInfo"			:	"EnableRunTimeTypeInfo",
		"OpenMP"				:	"OpenMPSupport",

		# Precompiled Headers
		"UsePrecompiledHeader"			:	"Create/UsePrecompiledHeader",

		# Output Files
		"ExpandAttributedSource"		:	"ExpandAttributedSource",
		"AssemblerOutput"			:	"AssemblerOutput",
		"GenerateXMLDocumentationFiles"		:	"GenerateXMLDocumentationFiles",

		# Browse Information
		"BrowseInformation"			:	"EnableBrowseInformation",

		# Advanced
		"CallingConvention"			:	"CallingConvention",
		"CompileAs"				:	"CompileAs",
		"UndefineAllPreprocessorDefinitions"	:	"UndefineAllPreprocessorDefinitions",
		"UseFullPaths"				:	"UseFullPaths",
		"OmitDefaultLibName"			:	"OmitDefaultLibraryNames",
		"ErrorReporting"			:	"ErrorReporting",
    },

	# Librarian
    "Librarian"   :
    {
		# General
		"SuppressStartupBanner"			:	"SuppressStartupBanner",
		"IgnoreAllDefaultLibraries"		:	"IgnoreAllDefaultLibraries",
		"UseUnicodeResponseFiles"		:	"UseUNICODEResponseFiles",
		"LinkLibraryDependencies"		:	"LinkLibraryDependencies",
    },

	# Linker
	"Linker"   :
    {
		# General
		"ShowProgress"				:	"ShowProgress",
		"LinkIncremental"			:	"EnableIncrementalLinking",
		"SuppressStartupBanner"			:	"SuppressStartupBanner",
		"IgnoreImportLibrary"			:	"IgnoreImportLibrary",
		"RegisterOutput"			:	"RegisterOutput",
		"LinkLibraryDependencies"		:	"LinkLibraryDependencies",
		"UseLibraryDependencyInputs"		:	"UseLibraryDependencyInputs",
		"UseUnicodeResponseFiles"		:	"UseUNICODEResponseFiles",

		# Input
		"IgnoreAllDefaultLibraries"		:	"IgnoreAllDefaultLibraries",

		# Manifest File
		"GenerateManifest"			:	"GenerateManifest",
		"AllowIsolation"			:	"AllowIsolation",

		# Debugging
		"GenerateDebugInformation"		:	"GenerateDebugInfo",
		"GenerateMapFile"			:	"GenerateMapFile",
		"MapExports"				:	"MapExports",
		"AssemblyDebug"				:	"DebuggableAssembly",

		# System
		"SubSystem"				:	"SubSystem",
		"LargeAddressAware"			:	"EnableLargeAddresses",
		"TerminalServerAware"			:	"TerminalServer",
		"SwapRunFromCD"				:	"SwapRunFromCD",
		"SwapRunFromNet"			:	"SwapRunFromNetwork",
		"Driver"				:	"Driver",

		# Optimization
		"OptimizeReferences"			:	"References",
		"EnableCOMDATFolding"			:	"EnableCOMDATFolding",
		"OptimizeForWindows98"			:	"OptimizeForWindows98", # lol
		"LinkTimeCodeGeneration"		:	"LinkTimeCodeGeneration",

		# Embedded IDL
		"IgnoreEmbeddedIDL"			:	"IgnoreEmbeddedIDL",

		# Advanced
		"ResourceOnlyDLL"			:	"NoEntryPoint",
		"SetChecksum"				:	"SetChecksum",
		"FixedBaseAddress"			:	"FixedBaseAddress",
		"TurnOffAssemblyGeneration"		:	"TurnOffAssemblyGeneration",
		"SupportUnloadOfDelayLoadedDLL"		:	"DelayLoadedDLL",
		"TargetMachine"				:	"TargetMachine",
		"Profile"				:	"Profile",
		"CLRThreadAttribute"			:	"CLRThreadAttribute",
		"CLRImageType"				:	"CLRImageType",
		"DelaySign"				:	"DelaySign",
		"ErrorReporting"			:	"ErrorReporting",
		"CLRUnmanagedCodeCheck"			:	"CLRUnmanagedCodeCheck",
    },

	"Resources" :
    {
		# General
		"Culture"				:	"Culture",
		"IgnoreStandardIncludePath"		:	"IgnoreStandardIncludePath",
		"ShowProgress"				:	"ShowProgress",
    }
}

# Convert the value of an option to the value vpc uses
ConfigOptions_ValueConvert = {

    "ConfigurationType"			:
    {		
        #"Makefile",
        #"Utility",
	    "StaticLibrary"	    :	"Static Library (.lib)",
	    "DynamicLibrary"	:	"Dynamic Library (.dll)",
	    "Application"	    : 	"Application (.exe)",
    },

    "UseOfMFC"				: 
    {
	    "0"	:	"Use Standard Windows Libraries",
	    "1"	:	"Use MFC In A Static Library",
	    "2"	:	"Use MFC In A Shared DLL",
    },

    "UseOfATL"				: 
    {
	    "0"	:	"Not Using ATL",
	    "1"	:	"Static Link To ATL",
	    "2"	:	"Dynamic Link To ATL",
    },

    "CharacterSet"				: 
    {
	    "0"	            :	"Not Set",
	    "Unicode"	    :	"Use Unicode Character Set",
	    "MultiByte" 	:	"Use Multi-Byte Character Set",
    },

    "WholeProgramOptimization"		:
    {
	    "false"	    :	"No",
	    "true"	    :	"Use Link Time Code Generation",
    },

    "WarningLevel"		:
    {
	    #"Level1"	    :	"Level 1 (/W1)",
	    #"Level2"	    :	"Level 2 (/W2)",
	    "Level3"	    :	"Level 3 (/W3)",
	    "Level4"	    :	"Level 4 (/W4)",
    },
}


# Adds a tag to a dictionary sorted in vpc's layout from the dictionaries above
def AddToVPCDict( tag, value, config_type ):

    global ConfigurationType

    if tag == "ConfigurationType":
        ConfigurationType = value

    # --------------------------------------------------------------------------------------
    # convert the single-value option names to what vpc uses

    for group in ConfigOptions_SingleValue.items():   
        if isinstance( group[1], dict ):
            for name_vcxproj, name_vpc in ConfigOptions_SingleValue[ group[0] ].items():
                if tag == name_vcxproj:
                    
                    # maybe the tag value needs to be converted
                    converted_value = False

                    if isinstance( group[1], dict ):
                        for option in ConfigOptions_ValueConvert.items():
                            if tag == option[0]:
                                for value_vcxproj, value_vpc in ConfigOptions_ValueConvert[ option[0] ].items():
                                    if value == value_vcxproj:
                                        VPC_Dict[ "Configuration" ][ config_type ][ group[0] ][ tag ] = value_vpc
                                        converted_value = True
                                        break

                    # did it convert the value?
                    if not converted_value:
                        # if not, add the original value
                        VPC_Dict[ "Configuration" ][ config_type ][ group[0] ][ name_vpc ] = value
                    break

                 # key[0]
        if tag == group[0]:
            VPC_Dict[ "Configuration" ][ config_type ][ group[1] ] = value

    # --------------------------------------------------------------------------------------
    # convert the multi-value option names to what vpc uses
    for group in ConfigOptions_MutliValue.items():

        if isinstance( group[1], dict ):
            for name_vcxproj, name_vpc in ConfigOptions_MutliValue[ group[0] ].items():
                if tag == name_vcxproj:

                    # get correct value
                    for option in ConfigOptions_ValueConvert.items():
                        if tag == option[0]:
                            for value_vcxproj, value_vpc in ConfigOptions_ValueConvert[ option[0] ].items():
                                if value == value_vcxproj:
                                    #[ group[1] ]
                                    VPC_Dict[ "Configuration" ][ config_type ][ group[0] ][ tag ] = value_vpc
                                    return


# Step One, Parse the xml stuff into a dictionary
def ConvertToVPCDictionary( vcxproj ):

    # -----------------------------------------------------------
    # Configuration
    
    PropertyGroups = vcxproj.findall( "PropertyGroup" )
    ItemDefinitionGroup = vcxproj.findall( "ItemDefinitionGroup" )

    # add auto dictionary adding to this with CreateDictionaryIfNotExist()
    for group in PropertyGroups:
        # could also try Condition, idk
        if bool( group.attrib ):
            if "Label" in group.attrib:
                Label = group.attrib[ "Label" ]

                if Label == "Configuration":

                    ConfigPlatform = group.attrib[ "Condition" ]

                    ConfigPlatform = ConfigPlatform.split( "'$(Configuration)|$(Platform)'==" )[1]

                    Config = ConfigPlatform.split( "|" )[0]
                    Config = Config.strip( "'" )

                else:
                    Config = "Shared"
            else:
                # it's probably condition then, unless it can hold another attribute
                if group.attrib[ "Condition" ].startswith( "'$(Configuration)|$(Platform)'" ):
                    ConfigPlatform = group.attrib[ "Condition" ]

                    ConfigPlatform = ConfigPlatform.split( "'$(Configuration)|$(Platform)'==" )[1]

                    Config = ConfigPlatform.split( "|" )[0]
                    Config = Config.strip( "'" )
        else:
            print( "uh" )

        for child in group:
            add_to_vpc = AddToVPCDict( child.tag, child.text, Config )



    for group in ItemDefinitionGroup:
        # could also try Condition, idk
        
        if group.attrib[ "Condition" ].startswith( "'$(Configuration)|$(Platform)'" ):
            ConfigPlatform = group.attrib[ "Condition" ]

            ConfigPlatform = ConfigPlatform.split( "'$(Configuration)|$(Platform)'==" )[1]

            Config = ConfigPlatform.split( "|" )[0]
            Config = Config.strip( "'" )

        for sub_group in group:

            for setting in sub_group:

                add_to_vpc = AddToVPCDict( setting.tag, setting.text, Config )


    # -----------------------------------------------------------
    # Project Files
        
    ItemGroup = vcxproj.findall( "ItemGroup" )

    # this is done because this could be in any order
    # ill probably have to make this a function this is needed other than just here
    index = 0
    for item in ItemGroup:
        test_cpp = ItemGroup[index].findall( "ClCompile" )
        test_h = ItemGroup[index].findall( "ClInclude" )

        if test_cpp != []:
            include_list_cpp = test_cpp

        if test_h != []:
            include_list_h = test_h

        # exit if they both have values
        if test_cpp != [] and test_h != []:
            break

        index += 1

    for File in include_list_cpp:
        File = File.attrib[ "Include" ]
        VPC_Dict[ "Project" ][ "Source Files" ].append( File )

    for File in include_list_h:
        File = File.attrib[ "Include" ]
        VPC_Dict[ "Project" ][ "Header Files" ].append( File )

    return VPC_Dict

def WriteProjectComment( vpc, project_name ):
    lines = []
    
    comment_bar = "//---------------------------------------------------------------------------------------------"

    lines.append( comment_bar )
    lines.append( "// **** AUTO-GENERATED: PLEASE FIX ANY ERRORS MANUALLY BEFORE USING THIS SCRIPT ****" )
    lines.append( "// " )
    lines.append( "// " + project_name )
    lines.append( "// " )
    lines.append( "// " + ConfigurationType + " Project Script" )
    lines.append( comment_bar )

    for line in lines:
        vpc.write( line + "\n" )

def WriteBasicVPCSettings( vpc, file_name ):
    lines = []

    # change this to automatically grab from where the project is compared to source folder?
    lines.append( '$Macro SRCDIR		    "..\.."' )

    if ConfigurationType == "StaticLibrary":
        lines.append( '$Macro OUTLIBNAME       "' + file_name + '"' )
        lines.append( '$Macro OUTLIBDIR        "$LIBPUBLIC"\n' )
        lines.append( '$Include "$SRCDIR\\vpc_scripts\\source_lib_base.vpc"\n' )

    elif ConfigurationType == "DynamicLibrary":
    
        lines.append( '$Macro OUTBINNAME       "' + file_name + '"' )
        lines.append( '$Macro OUTBINDIR        "$BINDEFAULT"\n' )
        lines.append( '$Include "$SRCDIR\\vpc_scripts\\source_dll_base.vpc"\n' )

    for line in lines:
        vpc.write( line + "\n" )

# ew
# last step
def WriteDictionaryToVPC( folder, file_name ):
    with open( folder + file_name + ".vpc", 'w', encoding="utf8" ) as vpc:

        # add the first standard lines of the vpc file
        WriteProjectComment( vpc, file_name )

        WriteBasicVPCSettings( vpc, file_name )

        # check if it's in the other config first (ex. Debug -> Release)
        CheckForDupeConfigSettings()

        # --------------------------------------------
        # Configuration Settings

        for group_tuple in VPC_Dict[ "Configuration" ].items():

            # can't delete stuff in a tuple, so ill just make an array lol
            group = []
            group.append( group_tuple[0] )
            group.append( group_tuple[1] )

            # is a group empty?
            for group_name, group_keys in list( group[1].items() ):
                if group_keys == {}:
                    del group[1][ group_name ]
                    continue
            
            # shared in-between, don't use a name for this
            if group[0] == "Shared":
                vpc.write( "$Configuration\n" )
            else:
                vpc.write( "$Configuration \"" + group[0] + '"\n' )

            vpc.write( "{\n" )

            WriteConfigurationSettingsToVPC( "Configuration", group, vpc )

            vpc.write( "}\n\n" )

        # --------------------------------------------
        # Project Files

        vpc.write( "$Project \"" + file_name + '"\n' )
        vpc.write( "{\n" )

        for group in VPC_Dict[ "Project" ].items():

            vpc.write( "    $Folder \"" + group[0] + '"\n' )
            vpc.write( "    {\n" )

            if group[0] == "Link Libraries":
                for file in group[1]:
                    # need to change this, what if we use $Lib for something?
                    vpc.write( "        $Lib \"" + file + "\"\n" )
            else:
                for file in group[1]:
                    # need to change this, what if we use $Lib for something?
                    vpc.write( "        $File \"" + file + "\"\n" )

            vpc.write( "    }\n\n" )

        vpc.write( "}\n\n" )
        

def ConvertDependenciesToProjectLibs( libraries ):
    libs_split = libraries.split( ";" )

    for lib in libs_split:

        lib_name = lib.split( ".lib" )[0]

        # don't add this, just return it at the end maybe?
        if lib_name == "%(AdditionalDependencies)":
            continue

        # check if it exists already
        if lib_name not in VPC_Dict[ "Project" ][ "Link Libraries" ]:
            VPC_Dict[ "Project" ][ "Link Libraries" ].append( lib_name )

def CheckForDupeConfigSettings():
    
    for group in VPC_Dict[ "Configuration" ].items():

        #if isinstance( VPC_Dict[ "Configuration" ][ group[0] ], dict ):
        if isinstance( group[1], dict ):
            # check if this exists in the other one
            if group[0] == "Debug" or group[0] == "Release":
                if group[0] == "Debug":
                    opp_dictionary = VPC_Dict[ "Configuration" ][ "Release" ]
                elif group[0] == "Release":
                    opp_dictionary = VPC_Dict[ "Configuration" ][ "Debug" ]

            # using list() allows me to delete parts of dictionaries in iterations
            for sub_group, sub_dict in list( group[1].items() ):
                for sub_key, sub_value in list( sub_dict.items() ):

                    if sub_key == "AdditionalDependencies":
                        ConvertDependenciesToProjectLibs( sub_value )

                        try:
                            del VPC_Dict[ "Configuration" ][ "Release" ][ sub_group ][ sub_key ]
                            del VPC_Dict[ "Configuration" ][ "Debug" ][ sub_group ][ sub_key ]
                            #del group[1][ sub_group ][ sub_key ]

                        except: pass  

                        VPC_Dict[ "Configuration" ][ "Shared" ][ sub_group ][ sub_key ] = "%(AdditionalDependencies)"
                        continue

                    for opp_group in list( opp_dictionary.items() ):
                        for opp_key, opp_value in list( opp_group[1].items() ):
                            if sub_key == opp_key and sub_value == opp_value:
                                # remove it from both and add it to shared

                                VPC_Dict[ "Configuration" ][ "Shared" ][ opp_group[0] ][ sub_key ] = sub_value

                                del VPC_Dict[ "Configuration" ][ "Release" ][ opp_group[0] ][ sub_key ]
                                del VPC_Dict[ "Configuration" ][ "Debug" ][ opp_group[0] ][ sub_key ]


def WriteConfigurationSettingsToVPC( root_dict_group, sub_dictionary, vpc ):
    for group in sub_dictionary[1].items():
        if isinstance( group[1], dict ):

            vpc.write( "    $" + group[0] + "\n" )
            vpc.write( "    {\n" )

            for sub_key, sub_value in group[1].items():
                vpc.write( "        $" + sub_key + "                \"" + sub_value + "\"\n" )

            vpc.write( "    }\n\n" )

        else:
            vpc.write( "    $" + group[0] + "        \"" + str( group[1] ) + "\"\n" )


def CreateDictionaryIfNotExist( dictionary, key, value_type ):

    try:
        dictionary[ key ]
    except KeyError:
        if value_type == "dict":
            dictionary[ key ] = {}
        elif value_type == "list":
            dictionary[ key ] = []
        elif value_type == "str":
            dictionary[ key ] = ""


def ConvertToVPC( vcxproj, folder, file_name ):

    ConvertToVPCDictionary( vcxproj )

    # ok now fucking die and write that dictionary
    WriteDictionaryToVPC( folder, file_name )    

# this runs first because this xmlns tag
# is added to the body name, which is really damn annoying
def RemoveXMLNSTag( folder, file ):

    with open( folder + file, 'r+', encoding="utf8" ) as vcxproj:

        data = [] #vcxproj.read()

        save_len = 0
        strip_len = 0
        
        found_xmlns = False

        for line in vcxproj:
            if found_xmlns == False:

                if "xmlns=" in line:
                    new_line = line.split( " xmlns=" )
                    new_line[1] = ">" + new_line[1].split( ">" )[1]
                    data.append( ''.join( new_line ) )
                    found_xmlns = True
                    
                    continue
                else:
                    save_len += len( line )
                    data.append( line )
            else:
                data.append( line )

        if found_xmlns == True:
            vcxproj.seek( save_len )
            vcxproj.truncate()

            for line in data:
                vcxproj.write( line )

    return

# ----------------------------------------------------------------------------
# starting point
folder = FindArgument( "-folder", True )

# use current directory if not specified
if not folder:
    folder = os.getcwd() + "\\"
elif not folder.endswith( "\\" ):
    folder = folder + "\\"

global ConfigurationType
ConfigurationType = ""

for file in os.listdir( folder ):

    if file.endswith(".vcxproj"):
    
        #print( "Converting: " + file )

        RemoveXMLNSTag( folder, file )

        # have to reset this every loop
        # could i clean up the configuration part at all?
        VPC_Dict = {
    
            "Configuration" :
            {
                "Debug" :
                {
                    "General"               : {},
                    "Compiler"              : {},
                    "Librarian"             : {},
                    "Linker"                : {},
                    "PreLinkEvent"          : {},
                    "PreBuildEvent"         : {},
                    "PostBuildEvent"        : {},
                    "CustomBuildStep"       : {},
                },

                "Release" :
                {
                    "General"               : {},
                    "Compiler"              : {},
                    "Librarian"             : {},
                    "Linker"                : {},
                    "PreLinkEvent"          : {},
                    "PreBuildEvent"         : {},
                    "PostBuildEvent"        : {},
                    "CustomBuildStep"       : {},
                },

                "Shared" :
                {
                    "General"               : {},
                    "Compiler"              : {},
                    "Librarian"             : {},
                    "Linker"                : {},
                    "PreLinkEvent"          : {},
                    "PreBuildEvent"         : {},
                    "PostBuildEvent"        : {},
                    "CustomBuildStep"       : {},
                },
            },
        
            "Project" :
            {
                "Source Files" : [],
                "Header Files" : [],
                "Link Libraries" : [],
            }
        }

        tree = ET.parse( folder + file )
        file_name = file.split( ".vcxproj" )[0]
        ConvertToVPC( tree, folder, file_name )

        print( "Finished Converting: " + file )