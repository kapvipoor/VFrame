#include "AssetLoader.h"
#include "Global.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "external/tiny_obj_loader.h"
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/tinygltf/tiny_gltf.h"

nm::float4 ComputeTangent(Vertex p_a, Vertex p_b, Vertex p_c)
{
	nm::float3 edge1 = p_b.pos - p_a.pos;
	nm::float3 edge2 = p_c.pos - p_a.pos;

	nm::float2 deltaUV1 = p_b.uv - p_a.uv;
	nm::float2 deltaUV2 = p_c.uv - p_a.uv;

	float f = 1.0f / ((deltaUV1.x() * deltaUV2.y()) - (deltaUV2.x() * deltaUV1.y()));

	nm::float4 tangent;
	tangent[0] = f * (deltaUV2.y() * edge1.x() - deltaUV1.y() * edge2.x());
	tangent[1] = f * (deltaUV2.y() * edge1.y() - deltaUV1.y() * edge2.y());
	tangent[2] = f * (deltaUV2.y() * edge1.z() - deltaUV1.y() * edge2.z());

	return tangent;
}

bool LoadRawImage(const char* p_path, ImageRaw& p_data)
{
	p_data.raw = stbi_load(p_path, &p_data.width, &p_data.height, &p_data.channels, STBI_rgb_alpha);
	
	// forcing this for now; as STBI Image returns the number of available channels in the image and not in laoded raw data; whcih is irrelevant for now
	p_data.channels = STBI_rgb_alpha;
		
	if (p_data.raw == nullptr)
	{
		std::cout << "stbi_load Failed - " << p_path << std::endl;
		return false;
	}

	return true;
}

void FreeRawImage(ImageRaw& p_data)
{
	stbi_image_free(p_data.raw);
	p_data = ImageRaw{};
}

bool LoadGltf(const char* p_path, SceneRaw& p_objScene, const ObjLoadData& p_loadData)
{
	tinygltf::Model input;
	tinygltf::TinyGLTF gltfContext;
	std::string error, warning;

	std::string strPath = std::string(p_path);
	std::size_t found = strPath.find_last_of("/");
	if (found == std::string::npos)
	{
		std::cerr << "Invalid gltf path - " << p_path << std::endl;
		return false;
	}
	std::string folderPath = strPath.substr(0, found);

	if (!gltfContext.LoadASCIIFromFile(&input, &error, &warning, p_path))
	{
		std::cout << "Failed to load Gltf - " << p_path << std::endl;
		return false;
	}

	bool fileNmae = GetFileName(p_path, p_objScene.name);
	uint32_t material_offset = (uint32_t)p_objScene.materialsList.size();
	uint32_t texture_offset = (uint32_t)p_objScene.textureList.size();

	MeshRaw objMesh;

	const tinygltf::Scene& scene = input.scenes[0];
	for (size_t n_id = 0; n_id < scene.nodes.size(); n_id++)
	{
		const tinygltf::Node node = input.nodes[scene.nodes[n_id]];

		objMesh.transform = nm::float4x4().identity();
		if (node.translation.size() == 3) {
			objMesh.transform = objMesh.transform * nm::translation(nm::float3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]));
		}
		if (node.rotation.size() == 4) {
			nm::quatf  q((float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2], (float)node.rotation[3]);
			nm::float4x4 q_mat = nm::quat2mat(q);
			objMesh.transform = objMesh.transform * q_mat;
		}
		if (node.scale.size() == 3) {
			objMesh.transform = objMesh.transform * nm::scale(nm::float4((float)node.scale[0], (float)node.scale[1], (float)node.scale[2], 1.0));
		}
		if (node.matrix.size() == 16) {
			objMesh.transform.from_rows(
				nm::float4((float)node.matrix[0], (float)node.matrix[1], (float)node.matrix[2], (float)node.matrix[3])
				, nm::float4((float)node.matrix[4], (float)node.matrix[5], (float)node.matrix[6], (float)node.matrix[7])
				, nm::float4((float)node.matrix[8], (float)node.matrix[9], (float)node.matrix[10], (float)node.matrix[11])
				, nm::float4((float)node.matrix[12], (float)node.matrix[13], (float)node.matrix[14], (float)node.matrix[15]));
		}

		nm::float4x4 translation = (node.translation.size() == 3) ? nm::translation(nm::float3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2])) : nm::translation(nm::float3(1.0f));
		nm::float4x4 scale = (node.scale.size() == 3) ? nm::scale(nm::float4((float)node.scale[0], (float)node.scale[1], (float)node.scale[2], 1.0f)) : nm::scale(nm::float4(1.0f));
		nm::float4x4 transform = translation * scale;

		if (node.mesh > -1)
		{
			const tinygltf::Mesh mesh = input.meshes[node.mesh];
			for (size_t i = 0; i < mesh.primitives.size(); i++)
			{
				const tinygltf::Primitive& glTFPrimitive = mesh.primitives[i];
				uint32_t firstIndex = static_cast<uint32_t>(objMesh.indicesList.size());
				uint32_t vertexStart = static_cast<uint32_t>(objMesh.vertexList.size());
				uint32_t indexCount = 0;
				nm::float3 bbMin{ 0, 0, 0 };
				nm::float3 bbMax{ 0, 0, 0 };

				//Vertices
				{
					const float* positionBuffer = nullptr;
					const float* normalsBuffer = nullptr;
					const float* texCoordsBuffer = nullptr;
					const float* tangentsBuffer = nullptr;
					size_t vertexCount = 0;

					// Get buffer data for vertex normals
					if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end()) {
						const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
						const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
						positionBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
						vertexCount = accessor.count;
					}
					// Get buffer data for vertex normals
					if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end()) {
						const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
						const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
						normalsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					}
					// Get buffer data for vertex texture coordinates
					// glTF supports multiple sets, we only load the first one
					if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end()) {
						const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
						const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
						texCoordsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					}
					// POI: This sample uses normal mapping, so we also need to load the tangents from the glTF file
					if (glTFPrimitive.attributes.find("TANGENT") != glTFPrimitive.attributes.end()) {
						const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TANGENT")->second];
						const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
						tangentsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					}

					// Append data to model's vertex buffer
					for (size_t v = 0; v < vertexCount; v++) {
						Vertex vert{};
						vert.pos = (transform * nm::float4(positionBuffer[(v * 3) + 0], positionBuffer[(v * 3) + 1], positionBuffer[(v * 3) + 2], 1.0f)).xyz();
						vert.normal = nm::float3(normalsBuffer ? nm::float3(normalsBuffer[(v * 3) + 0], normalsBuffer[(v * 3) + 1], normalsBuffer[(v * 3) + 2]) : nm::float3(0.0f));
						vert.uv = texCoordsBuffer ? nm::float2(texCoordsBuffer[(v * 2) + 0], texCoordsBuffer[(v * 2) + 1]) : nm::float2(0.0f);
						vert.tangent = tangentsBuffer ? nm::float4(tangentsBuffer[(v * 4) + 0], tangentsBuffer[(v * 4) + 1], tangentsBuffer[(v * 4) + 2], tangentsBuffer[(v * 4) + 3]) : nm::float4(0.0);
						//vert.color = nm::float3(1.0, 1.0, 1.0);

						bbMin[0] = std::min(bbMin[0], vert.pos[0]);
						bbMin[1] = std::min(bbMin[1], vert.pos[1]);
						bbMin[2] = std::min(bbMin[2], vert.pos[2]);

						bbMax[0] = std::max(bbMax[0], vert.pos[0]);
						bbMax[1] = std::max(bbMax[1], vert.pos[1]);
						bbMax[2] = std::max(bbMax[2], vert.pos[2]);

						objMesh.vertexList.push_back(vert);
					}
				}

				{
					const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.indices];
					const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];

					indexCount += static_cast<uint32_t>(accessor.count);

					// glTF supports different component types of indices
					switch (accessor.componentType) {
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
						const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
						for (size_t index = 0; index < accessor.count; index++) {
							objMesh.indicesList.push_back(buf[index] + vertexStart);
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
						const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
						for (size_t index = 0; index < accessor.count; index++) {
							objMesh.indicesList.push_back(buf[index] + vertexStart);
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
						const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
						for (size_t index = 0; index < accessor.count; index++) {
							objMesh.indicesList.push_back(buf[index] + vertexStart);
						}
						break;
					}
					default:
						std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
						return false;
					}
				}

				Submesh submesh{};
				submesh.firstIndex	= firstIndex;
				submesh.indexCount	= indexCount;
				submesh.materialId	= material_offset + glTFPrimitive.material;
				submesh.bbox.bbMin	= bbMin;
				submesh.bbox.bbMax	= bbMax;
				objMesh.submeshes.push_back(submesh);

				objMesh.bbox.bbMin	= nm::float3{0.0f, 0.0f, 0.0f};
				objMesh.bbox.bbMin[0]	= std::min(objMesh.bbox.bbMin[0], submesh.bbox.bbMin[0]);
				objMesh.bbox.bbMin[1]	= std::min(objMesh.bbox.bbMin[1], submesh.bbox.bbMin[1]);
				objMesh.bbox.bbMin[2]	= std::min(objMesh.bbox.bbMin[2], submesh.bbox.bbMin[2]);

				objMesh.bbox.bbMax		= nm::float3{ 0.0f, 0.0f, 0.0f };
				objMesh.bbox.bbMax[0]	= std::max(objMesh.bbox.bbMax[0], submesh.bbox.bbMax[0]);
				objMesh.bbox.bbMax[1]	= std::max(objMesh.bbox.bbMax[1], submesh.bbox.bbMax[1]);
				objMesh.bbox.bbMax[2]	= std::max(objMesh.bbox.bbMax[2], submesh.bbox.bbMax[2]);
			}
		}
	}

	p_objScene.meshList.push_back(objMesh);

	// load materials
	for (auto& gltf_mat : input.materials)
	{
		Material mat{ MAX_SUPPORTED_TEXTURES, MAX_SUPPORTED_TEXTURES };
		if (gltf_mat.values.find("baseColorTexture") != gltf_mat.values.end())
		{
			mat.color_id = texture_offset + gltf_mat.values["baseColorTexture"].TextureIndex();
		}

		if (gltf_mat.additionalValues.find("normalTexture") != gltf_mat.additionalValues.end())
		{
			mat.normal_id = texture_offset + gltf_mat.additionalValues["normalTexture"].TextureIndex();
		}

		p_objScene.materialsList.push_back(mat);
	}

	// load images
	for (const auto& image : input.images)
	{
		ImageRaw iraw{ nullptr, 0, 0, 0 };
		std::string path = (folderPath + "/" + image.uri);
		iraw.raw = stbi_load(path.c_str(), &iraw.width, &iraw.height, &iraw.channels, STBI_rgb_alpha);
		if (iraw.raw == nullptr)
		{
			std::cout << "stbi_load Failed - " << path << std::endl;
			return false;
		}

		iraw.channels = STBI_rgb_alpha;
		p_objScene.textureList.push_back(iraw);
	}

	if (input.images.empty())
	{
		ImageRaw iraw{ nullptr, 0, 0, 0 };
		p_objScene.textureList.push_back(iraw);
	}

	return true;
}

bool LoadObj(const char* p_path, SceneRaw& p_objScene, const ObjLoadData& p_loadData)
{
	std::string strPath = std::string(p_path);
	std::size_t found = strPath.find_last_of("/");
	if (found == std::string::npos)
	{
		std::cerr << "Invalid Obj path - " << p_path << std::endl;
		return false;
	}
	std::string folderPath = strPath.substr(0, found);

	tinyobj::ObjReaderConfig reader_config;
	tinyobj::ObjReader objReader;

	if (!objReader.ParseFromFile(p_path, reader_config))
	{
		if (!objReader.Error().empty())
		{
			std::cerr << "TinyObjReader: " << objReader.Error();
		}
		return false;
	}

	if (!objReader.Warning().empty())
	{
		std::cout << "TinyObjReader Warning: " << objReader.Warning();
	}

	auto& attrib = objReader.GetAttrib();
	auto& shapes = objReader.GetShapes();

	MeshRaw objMesh;
	auto vertList = &objMesh.vertexList;
	auto indList = &objMesh.indicesList;

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	int meshCount = 0;
	for (const auto& shape : shapes)
	{
		{
			int i = 0;
			int valid_i = 0;
			for (const auto& index : shape.mesh.indices)
			{
				Vertex vertex;

				vertex.pos = nm::float3{
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.normal = nm::float3{
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};

				int in = index.texcoord_index;
				if (in < 0)
					in = 0;

				vertex.uv = nm::float2{
						attrib.texcoords[2 * in + 0],
						attrib.texcoords[2 * in + 1] };

				if (p_loadData.flipUV == true)
				{
					vertex.uv[1] = 1.0f - vertex.uv[1];
				}

				//vertex.color = nm::float3{1.0f, 1.0f, 1.0f
					/*attrib.colors[3 * index.vertex_index + 0],
					attrib.colors[3 * index.vertex_index + 1],
					attrib.colors[3 * index.vertex_index + 2]*/
				//};

				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(vertList->size());
					vertList->push_back(vertex);
				}

				indList->push_back(uniqueVertices[vertex]);
				i++;
			}
		}
		meshCount++;
	}
	p_objScene.meshList.push_back(objMesh);

	if (p_loadData.loadMeshOnly == true)
		return true;

	auto& materials = objReader.GetMaterials();
	for (auto& material : materials)
	{
		int width, height, channels;
		// loading diffuse textures
		{
			ImageRaw diffuse{ nullptr, 0, 0, 0 };		// initilizing diffuse raw tex
			if (!material.diffuse_texname.empty())
			{
				std::string diffusePath = folderPath + "/" + material.diffuse_texname.c_str();
				diffuse.raw = stbi_load(diffusePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
				diffuse.width = width;
				diffuse.height = height;
				diffuse.channels = channels;
			}
			else
				std::cout << "Diffuse Mat Not Found: " << material.name << std::endl;

			p_objScene.textureList.push_back(diffuse);
		}
		{
			ImageRaw normal{ nullptr, 0, 0, 0 };	// initilizind normal raw tex
			if (!material.bump_texname.empty())
			{
				std::string normalPath = folderPath + "/" + material.bump_texname.c_str();
				normal.raw = stbi_load(normalPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
				normal.width = width;
				normal.height = height;
				normal.channels = channels;
			}
			else
			{
				std::cout << "Nomal Mat Not Found: " << material.name << std::endl;
			}

			p_objScene.textureList.push_back(normal);
		}


	}

	return true;
}