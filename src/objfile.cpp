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
#include "objfile.h"
#include "config.h"

std::string ObjFace::toString() const
{
	return boost::join(vertexIndices
		| boost::adaptors::transformed([](int i) { return std::to_string(i); }), " ");
}

ObjFile::ObjFile()
{
}

ObjFile::~ObjFile()
{
}

bool ObjFile::load(const std::string& filename)
{
	if (filename.empty()) return false;

	FILE* file = fopen(filename.c_str(), "rb");

	if (!file) return false;

	char buffer[256];

	std::string currentMaterial, currentGroup, currentObject;

	while (fgets(buffer, sizeof(buffer), file))
	{
		std::string line = buffer;

		// ignore empty lines
		if (line.length() < 3) continue;

		auto headerLength = line.find(' ');

		// no spaces
		if (headerLength == std::string::npos) continue;

		// read 2 first bytes to check type of the line
		std::string header = line.substr(0, headerLength);

		// rest is the content, but remove EOL
		auto contentLength = line.find_last_not_of("\n\r\t ");

		std::string content = line.substr(headerLength + 1, contentLength - headerLength);

		if (header == "v")
		{
			// precision 0.001 as DAZ Studio
			ObjVertex v;

			// current object for vertex
			v.object = currentObject;

			// string version
			v.string = content;

			// doubles version
			if (sscanf(content.c_str(), "%lf %lf %lf", &v.x, &v.y, &v.z) == 3)
			{
				// vertex, just put the string, because we'll just compare them later
				m_vertices.push_back(v);
			}
		}
		else if (header == "f")
		{
			parseFace(content, currentMaterial, currentGroup);
		}
		else if (header == "usemtl")
		{
			if (m_material.empty())
			{
				m_material = content;
			}

			currentMaterial = content;
		}
		else if (header == "o")
		{
			if (m_name.empty())
			{
				m_name = content;
			}

			currentObject = content;
		}
		else if (header == "g")
		{
			currentGroup = content;
		}
	}

	fclose(file);

	return true;
}

bool ObjFile::save(const std::string& filename) const
{
	if (filename.empty()) return false;

	FILE* file = fopen(filename.c_str(), "wb");

	if (!file) return false;

	fprintf(file, "# Kervala's OBJTool v%s File\n", VERSION);

	std::string currentObject;

	for (const ObjVertex& vertex: m_vertices)
	{
		if (!vertex.object.empty() && vertex.object != currentObject)
		{
			fprintf(file, "o %s\n", vertex.object.c_str());

			currentObject = vertex.object;
		}

		// reuse the string version
		fprintf(file, "v %s\n", vertex.string.c_str());
	}

	fprintf(file, "usemtl %s\n", m_material.c_str());
	fprintf(file, "s off\n");

	std::string currentMaterial, currentGroup;

	for (const ObjFace face : m_faces)
	{
		if (!face.material.empty() && face.material != currentMaterial)
		{
			fprintf(file, "usemtl %s\n", face.material.c_str());

			currentMaterial = face.material;
		}

		fprintf(file, "f %s\n", face.toString().c_str());
	}

	fclose(file);

	return true;
}

IndicesList ObjFile::getDifferentVertices(const ObjFile& other) const
{
	IndicesList indices;

	int verticesCount = std::min((int)m_vertices.size(), (int)other.m_vertices.size());

	for (int i = 0; i < verticesCount; ++i)
	{
		if (!m_vertices[i].is_close(other.m_vertices[i]))
		{
			indices.push_back(i + 1);
		}
	}

	return indices;
}

bool ObjFile::createVerticesCache()
{
	if (!m_cachedIndices.empty()) return true;

	for (int i = 0; i < m_faces.size(); ++i)
	{
		IndicesList faceVertexIndices = m_faces[i].vertexIndices;

		for (int faceVertexIndex : faceVertexIndices)
		{
			m_cachedIndices[faceVertexIndex].push_back(i + 1);
		}
	}

	return true;
}

ObjFile ObjFile::getDifferences(const ObjFile& other) const
{
	ObjFile file;

	file.m_name = other.m_name;
	file.m_material = other.m_material;

	IndicesList oldVertices = getDifferentVertices(other);

	// index = old, value = new
	IndicesMap newVertices;

	// put vertices that are different
	for (int oldVertex : oldVertices)
	{
		file.m_vertices.push_back(other.m_vertices[oldVertex - 1]);

		newVertices[oldVertex] = (int)file.m_vertices.size();

		IndicesList oldFaces = getFacesUsingVertex(oldVertex);

		for (int oldFaceIndex : oldFaces)
		{
			ObjFace newFace;

			newFace.material = m_faces[oldFaceIndex - 1].material;

			// vertex indices fromd old face
			IndicesList oldVerticesFace = m_faces[oldFaceIndex - 1].vertexIndices;

			for (int oldVertexFace : oldVerticesFace)
			{
				int newVertexFace;

				IndicesMap::const_iterator it = newVertices.find(oldVertexFace);

				if (it != newVertices.cend())
				{
					// new vertex already in vertices list, convert the index
					newVertexFace = it->second;
				}
				else
				{
					// not in vertices list, add it
					file.m_vertices.push_back(other.m_vertices[oldVertexFace - 1]);

					newVertexFace = (int)file.m_vertices.size();

					newVertices[oldVertexFace] = newVertexFace;
				}

				newFace.vertexIndices.push_back(newVertexFace);
			}

			file.m_faces.push_back(newFace);
		}
	}

	return file;
}

bool ObjFile::colorizeDifferences(const std::string& material, const ObjFile& other)
{
	IndicesList vertices = getDifferentVertices(other);

	if (vertices.empty()) return false;

	// put vertices that are different
	for (int vertex : vertices)
	{
		IndicesList faces = getFacesUsingVertex(vertex);

		for (int face : faces)
		{
			m_faces[face - 1].material = material;
		}
	}

	return true;
}

bool ObjFile::mergeFacesByMaterial(const std::string& material, const ObjFile& other)
{
	if (material.empty()) return false;

	IndicesList faces = other.getFacesByMaterial(material);

	if (faces.empty()) return false;

	for (int face : faces)
	{
		IndicesList vertices = other.m_faces[face - 1].vertexIndices;

		for (int vertex : vertices)
		{
			// same index in both OBJs
			m_vertices[vertex - 1] = other.m_vertices[vertex - 1];
		}
	}

	return true;
}

bool ObjFile::haveSameVerticesCount(const ObjFile& other) const
{
	return m_vertices.size() == other.m_vertices.size();
}

bool ObjFile::haveSameFacesCount(const ObjFile& other) const
{
	return m_faces.size() == other.m_faces.size();
}

IndicesList ObjFile::getFacesByMaterial(const std::string& material) const
{
	IndicesList indices;

	for (int i = 0; i < m_faces.size(); ++i)
	{
		ObjFace face = m_faces[i];

		if (face.material == material)
		{
			indices.push_back(i + 1);
		}
	}

	return indices;
}

IndicesList ObjFile::getFacesUsingVertex(int vertexIndex) const
{
	if (!m_cachedIndices.empty())
	{
		VerticesFacesList::const_iterator it = m_cachedIndices.find(vertexIndex);

		if (it != m_cachedIndices.cend())
		{
			return it->second;
		}

		return IndicesList();
	}

	IndicesList indices;

	for (int i = 0; i < m_faces.size(); ++i)
	{
		IndicesList faceVertexIndices = m_faces[i].vertexIndices;

		IndicesList::const_iterator it = std::find(faceVertexIndices.cbegin(), faceVertexIndices.cend(), vertexIndex);

		if (it != faceVertexIndices.cend())
		{
			indices.push_back(i + 1);
		}
	}

	return indices;
}

bool ObjFile::parseFace(const std::string& content, const std::string& material, const std::string& group)
{
	std::vector<std::string> tokens;
	boost::split(tokens, content, [](char c) {return c == ' '; });

	if (tokens.empty()) return false;

	ObjFace face;

	for (const std::string& token : tokens)
	{
		auto index = token.find_first_of('/');

		int vertexIndex;

		if (index != std::string::npos)
		{
			// first value
			vertexIndex = std::stoi(token.substr(0, index));
		}
		else
		{
			// only vertex index
			vertexIndex = std::stoi(token);
		}

		face.vertexIndices.push_back(vertexIndex);
	}

	face.material = material;
	face.group = group;

	m_faces.push_back(face);

	return true;
}
