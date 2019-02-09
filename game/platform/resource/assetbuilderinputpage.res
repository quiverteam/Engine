"//PLATFORM/resource/assetbuilderinputpage.res"
{
	"InputPage"
	{
		"ControlName"	"PropertyPage"
		"fieldName"		"InputPage"
		"xpos"			"6"
		"ypos"			"6"
		"wide"			"256"
		"tall"			"256"
		"AutoResize"	"0"
		"PinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
			
		"PropertiesSplitter"
		{
			"ControlName"	"Splitter"
			"fieldName"		"PropertiesSplitter"
			"visible"		"1"
			"enabled"		"1"
			"pinCorner"		"0"
			"AutoResize"	"3"
			"PinnedCornerOffsetX" "0"
			"PinnedCornerOffsetY" "0"
			"UnpinnedCornerOffsetX" "0"
			"UnpinnedCornerOffsetY" "0"
			"splitter0" "192"
			
			"child0"
			{		
				"SourcesList"
				{
					"ControlName"	"ListPanel"
					"fieldName"		"SourcesList"
					"xpos"			"6"
					"ypos"			"6"
					"wide"			"256"
					"tall"			"212"
					"AutoResize"	"3"
					"PinCorner"		"0"
					"UnpinnedCornerOffsetX" "-6"
					"UnpinnedCornerOffsetY" "-6"
					"visible"		"1"
					"enabled"		"1"
					"tabPosition"	"0"
				}
			}
			
			"child1"
			{
				"CompileOptionsLabel"
				{
					"ControlName"	"Label"
					"fieldName"		"CompileOptionsLabel"
					"xpos"			"12"
					"ypos"			"6"
					"wide"			"100"
					"tall"			"24"
					"autoResize"	"0"
					"pinCorner"		"0"
					"visible"		"1"
					"enabled"		"1"
					"tabPosition"	"0"
					"labelText"		"Compile Options"
					"textAlignment"	"west"
					"dulltext"		"1"
					"brighttext"	"0"
				}

				"CompileOptions"
				{
					"ControlName"	"CDmePanel"
					"fieldName"		"CompileOptions"
					"xpos"			"6"
					"ypos"			"30"
					"wide"			"256"
					"tall"			"212"
					"AutoResize"	"3"
					"PinCorner"		"0"
					"UnpinnedCornerOffsetX" "-6"
					"UnpinnedCornerOffsetY" "-6"
					"visible"		"1"
					"enabled"		"1"
					"tabPosition"	"0"
				}
			}
		}
	}
}
