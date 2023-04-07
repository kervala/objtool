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

#ifndef OBJFILE_H
#define OBJFILE_H

#include <vector>
#include <string>
#include <map>

inline bool is_close(double a, double b)
{
	return std::fabs(a - b) < 1e-3;
}

typedef std::vector<int> IndicesList;
typedef std::map<int, int> IndicesMap;

struct ObjVertex
{
	double x, y, z;
	std::string string;
	std::string object;

	bool is_close(const ObjVertex& other) const
	{
		return ::is_close(x, other.x) && ::is_close(y, other.y) && ::is_close(z, other.z);
	}
};

struct ObjFace
{
	std::string group;
	std::string material;
	IndicesList vertexIndices;

	std::string toString() const;
};

// key = vertex index, value = faces indices
typedef std::map<int, IndicesList> VerticesFacesList;

class ObjFile
{
public:
	ObjFile();
	virtual ~ObjFile();

	bool load(const std::string& filename);
	bool save(const std::string& filename) const;

	IndicesList getFacesUsingVertex(int vertexIndex) const;

	IndicesList getDifferentVertices(const ObjFile& other) const;

	IndicesList getFacesByMaterial(const std::string& material) const;

	bool parseFace(const std::string& content, const std::string& surface, const std::string& group);

	ObjFile getDifferences(const ObjFile& other) const;
	bool colorizeDifferences(const std::string& material, const ObjFile& other);
	bool addMaterialsFrom(const ObjFile& other);

	bool mergeFacesByMaterial(const std::string& material, const ObjFile& other);

	bool haveSameVerticesCount(const ObjFile& other) const;
	bool haveSameFacesCount(const ObjFile& other) const;

	bool createVerticesCache();

	std::vector<ObjVertex> m_vertices;
	std::vector<ObjFace> m_faces;

	VerticesFacesList m_cachedIndices;
};

#endif
