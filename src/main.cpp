/*
 *  OBJTool is a tool to manage OBJ files
 *  Copyright (C) 2023  Kervala
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common.h"
#include "config.h"

#include "objfile.h"

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("%s %s by %s\n", PRODUCT, VERSION, AUTHOR);
		printf("\n");

		printf("Syntax: %s [options] <input filename> <output filename>\n", argv[0]);
		printf("\n");

		printf("Program options:\n");
		printf("  -d, --diff <filename>                  create an OBJ file with only different faces from filename\n");
		printf("  -c, --colordiff <filename> <material>  create an OBJ file with different faces from filename with another material\n");
		printf("  -m, --merge <filename> <material>      merge faces with the specific material from file with input filename\n");
		printf("  -a, --addmaterials <filename>          add materials from filename and copy them to input file\n");
		printf("  -s, --simplify                         only keep geometry data\n");

		return 1;
	}

	std::string inputFilename1; // required
	std::string inputFilename2;

	std::string outputFilename; // required
	std::string material;

	char command = '\0';

	for (int i = 1; i < argc; ++i)
	{
		std::string option = argv[i];

		if (option.empty()) continue;

		if (option[0] == '-')
		{
			option = option.substr(option.find_first_not_of('-'));

			if (option == "d" || option == "diff")
			{
				inputFilename2 = argv[++i];
			}
			if (option == "a" || option == "addmaterials")
			{
				inputFilename2 = argv[++i];
			}
			else if (option == "c" || option == "colordiff" || option == "m" || option == "merge")
			{
				inputFilename2 = argv[++i];
				material = argv[++i];
			}
			else if (option == "s" || option == "simplify")
			{
			}
			else
			{
				printf("Unknown command %s\n", option.c_str());

				return 1;
			}

			command = option[0];
		}
		else if (inputFilename1.empty())
		{
			inputFilename1 = option;
		}
		else if (outputFilename.empty())
		{
			outputFilename = option;
		}
	}

	if (inputFilename1.empty())
	{
		printf("No input filename\n");

		return 1;
	}

	if (outputFilename.empty())
	{
		printf("No output filename\n");

		return 1;
	}

	if (!command)
	{
		printf("No command\n");

		return 1;
	}

	ObjFile obj;

	obj.load(inputFilename1);

	if (command == 'c' || command == 'd' || command == 'm' || command == 'a')
	{
		ObjFile obj2;

		obj2.load(inputFilename2);

		if (!obj.haveSameFacesCount(obj2))
		{
			printf("Files don't have the same number of faces! %zu != %zu\n", obj.m_faces.size(), obj2.m_faces.size());
			return 1;
		}

		if (!obj.haveSameVerticesCount(obj2))
		{
			printf("Files don't have the same number of vertices! %zu != %zu\n", obj.m_vertices.size(), obj2.m_vertices.size());
			return 1;
		}

		// optimisation
		obj.createVerticesCache();

		if (command == 'd')
		{
			ObjFile out = obj.getDifferences(obj2);
			out.save(outputFilename);
		}
		else if (command == 'a')
		{
			obj.addMaterialsFrom(obj2);
			obj.save(outputFilename);
		}
		else if (command == 'c')
		{

			obj.colorizeDifferences(material, obj2);
			obj.save(outputFilename);
		}
		else if (command == 'm')
		{
			if (!obj.mergeFacesByMaterial(material, obj2))
			{
				printf("No faces with material '%s'!\n", material.c_str());
				return 1;
			}

			obj.save(outputFilename);
		}
	}
	else if (command == 's')
	{
		obj.save(outputFilename);
	}

	return 0;
}
