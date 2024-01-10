#include "AssetLoader.h"
#include "Global.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "external/tiny_obj_loader.h"
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/tinygltf/tiny_gltf.h"
#include <fstream>

nm::float4 ComputeTangent(Vertex p_a, Vertex p_b, Vertex p_c)
{
	float* pos_a = p_a.GetAttribute(Vertex::AttributeFlag::position);
	float* pos_b = p_b.GetAttribute(Vertex::AttributeFlag::position);
	float* pos_c = p_c.GetAttribute(Vertex::AttributeFlag::position);
	
	nm::float3 edge1 = nm::float3(pos_b[0] - pos_a[0], pos_b[1] - pos_a[1], pos_b[2] - pos_a[2]); //p_b.position - p_a.position;
	nm::float3 edge2 = nm::float3(pos_c[0] - pos_a[0], pos_c[1] - pos_a[1], pos_c[2] - pos_a[2]); //p_c.position - p_a.position;

	float* uv_a = p_a.GetAttribute(Vertex::AttributeFlag::uv);
	float* uv_b = p_b.GetAttribute(Vertex::AttributeFlag::uv);
	float* uv_c = p_c.GetAttribute(Vertex::AttributeFlag::uv);

	nm::float2 deltaUV1 = nm::float2(uv_b[0] - uv_a[0], uv_b[1] - uv_a[1]); //p_b.attributes[1].uv - p_a.attributes[1].uv;
	nm::float2 deltaUV2 = nm::float2(uv_c[0] - uv_a[0], uv_c[1] - uv_a[1]); //p_c.attributes[1].uv - p_a.attributes[1].uv;

	float f = 1.0f / ((deltaUV1.x() * deltaUV2.y()) - (deltaUV2.x() * deltaUV1.y()));

	nm::float4 tangent;
	tangent[0] = f * (deltaUV2.y() * edge1.x() - deltaUV1.y() * edge2.x());
	tangent[1] = f * (deltaUV2.y() * edge1.y() - deltaUV1.y() * edge2.y());
	tangent[2] = f * (deltaUV2.y() * edge1.z() - deltaUV1.y() * edge2.z());

	return tangent;
}

void ComputeBBox(BBox& p_bbox)
{
	p_bbox.bBox[0] = nm::float3{ p_bbox.bbMin[0], p_bbox.bbMin[1], p_bbox.bbMin[2] };
	p_bbox.bBox[1] = nm::float3{ p_bbox.bbMax[0], p_bbox.bbMin[1], p_bbox.bbMin[2] };
	p_bbox.bBox[2] = nm::float3{ p_bbox.bbMax[0], p_bbox.bbMax[1], p_bbox.bbMin[2] };
	p_bbox.bBox[3] = nm::float3{ p_bbox.bbMin[0], p_bbox.bbMax[1], p_bbox.bbMin[2] };
	p_bbox.bBox[4] = nm::float3{ p_bbox.bbMin[0], p_bbox.bbMin[1], p_bbox.bbMax[2] };
	p_bbox.bBox[5] = nm::float3{ p_bbox.bbMax[0], p_bbox.bbMin[1], p_bbox.bbMax[2] };
	p_bbox.bBox[6] = nm::float3{ p_bbox.bbMax[0], p_bbox.bbMax[1], p_bbox.bbMax[2] };
	p_bbox.bBox[7] = nm::float3{ p_bbox.bbMin[0], p_bbox.bbMax[1], p_bbox.bbMax[2] };
}

void GenerateSphere(int p_stackCount, int p_sectorCount, RawSphere& p_sphere, float p_radius)
{
	p_sphere.indices.clear();
	p_sphere.lineIndices.clear();

	float x, y, z, xy;									// vertex position
	
	float sectorStep = 2 * nm::PI / p_sectorCount;
	float stackStep = nm::PI / p_stackCount;
	float sectorAngle, stackAngle;

	// generate CCW index list of sphere triangles
	// k1--k1+1
	// |  / |
	// | /  |
	// k2--k2+1
	int k1, k2;

	for (int i = 0; i <= p_stackCount; ++i)
	{
		stackAngle = nm::PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
		xy = p_radius * cosf(stackAngle);             // r * cos(u)
		z = p_radius * sinf(stackAngle);              // r * sin(u)

		k1 = i * (p_stackCount + 1);     // beginning of current stack
		k2 = k1 + p_stackCount + 1;      // beginning of next stack


		// add (sectorCount+1) vertices per stack
		// the first and last vertices have same position and normal, but different texture coordinates
		for (int j = 0; j <= p_sectorCount; ++j, ++k1, ++k2)
		{
			sectorAngle = j * sectorStep;           // starting from 0 to 2pi

			// vertex position (x, y, z)
			x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
			y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
			//p_sphere.vertices.push_back(nm::float3(x, y, z));
			p_sphere.vertices.AddVertex(Vertex::AttributeFlag::position, &nm::float3{ x, y, z }[0]);

			// 2 triangles per sector excluding first and last stacks
			// k1 => k2 => k1+1
			if(i != 0)
			{
			    p_sphere.indices.push_back(k1);
			    p_sphere.indices.push_back(k2);
			    p_sphere.indices.push_back(k1 + 1);
			}

			// k1+1 => k2 => k2+1
			if(i != (p_stackCount -1))
			{
			    p_sphere.indices.push_back(k1 + 1);
			    p_sphere.indices.push_back(k2);
			    p_sphere.indices.push_back(k2 + 1);
			}

			// store indices for lines
			// vertical lines for all stacks, k1 => k2
			p_sphere.lineIndices.push_back(k1);
			p_sphere.lineIndices.push_back(k2);
			if(i != 0)  // horizontal lines except 1st stack, k1 => k+1
			{
			    p_sphere.lineIndices.push_back(k1);
			    p_sphere.lineIndices.push_back(k1 + 1);
			}
		}
	}
}

bool LoadRawImage(const char* p_path, ImageRaw& p_data)
{
	p_data.raw = stbi_load(p_path, &p_data.width, &p_data.height, &p_data.channels, STBI_rgb_alpha);
	
	// forcing this for now; as STBI Image returns the number of available channels in the image and not in loaded raw data; which is irrelevant for now
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

	uint32_t material_offset = (uint32_t)p_objScene.materialsList.size();
	uint32_t texture_offset = (uint32_t)p_objScene.textureList.size();

	MeshRaw objMesh;
	objMesh.vertexList = VertexList(Vertex::AttributeFlag::position | Vertex::AttributeFlag::normal | Vertex::AttributeFlag::uv | Vertex::AttributeFlag::tangent);
	bool fileName = GetFileName(p_path, objMesh.name);
	if (!fileName)
	{
		std::cout << "Failed to Get Filename - " << p_path << std::endl;
		return false;
	}

	const tinygltf::Scene& scene = input.scenes[0];
	for (size_t n_id = 0; n_id < scene.nodes.size(); n_id++)
	{
		const tinygltf::Node node = input.nodes[scene.nodes[n_id]];

		objMesh.transform = nm::float4x4().identity();
		//if (node.translation.size() == 3) {
		//	objMesh.transform = objMesh.transform * nm::translation(nm::float3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]));
		//}
		//if (node.rotation.size() == 4) {
		//	nm::quatf  q((float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2], (float)node.rotation[3]);
		//	nm::float4x4 q_mat = nm::quat2mat(q);
		//	objMesh.transform = objMesh.transform * q_mat;
		//}
		////if (node.scale.size() == 3) {
		////	objMesh.transform = objMesh.transform * nm::scale(nm::float4((float)node.scale[0], (float)node.scale[1], (float)node.scale[2], 1.0));
		////}
		//if (node.matrix.size() == 16) {
		//	objMesh.transform.from_rows(
		//		nm::float4((float)node.matrix[0], (float)node.matrix[1], (float)node.matrix[2], (float)node.matrix[3])
		//		, nm::float4((float)node.matrix[4], (float)node.matrix[5], (float)node.matrix[6], (float)node.matrix[7])
		//		, nm::float4((float)node.matrix[8], (float)node.matrix[9], (float)node.matrix[10], (float)node.matrix[11])
		//		, nm::float4((float)node.matrix[12], (float)node.matrix[13], (float)node.matrix[14], (float)node.matrix[15]));
		//}

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
				uint32_t vertexStart = static_cast<uint32_t>(objMesh.vertexList.size()/objMesh.vertexList.GetVertexSize());
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

					for (size_t v = 0; v < vertexCount; v++) {
						Vertex vert = objMesh.vertexList.CreateVertex();
						vert.AddAttribute(Vertex::AttributeFlag::position, &((transform * nm::float4(positionBuffer[(v * 3) + 0], positionBuffer[(v * 3) + 1], positionBuffer[(v * 3) + 2], 1.0f)).xyz())[0]);
						vert.AddAttribute(Vertex::AttributeFlag::normal, &nm::float3(normalsBuffer ? nm::float3(normalsBuffer[(v * 3) + 0], normalsBuffer[(v * 3) + 1], normalsBuffer[(v * 3) + 2]) : nm::float3(0.0f))[0]);
						vert.AddAttribute(Vertex::AttributeFlag::uv, &(texCoordsBuffer ? nm::float2(texCoordsBuffer[(v * 2) + 0], texCoordsBuffer[(v * 2) + 1]) : nm::float2(0.0f))[0]);
						vert.AddAttribute(Vertex::AttributeFlag::tangent, &(tangentsBuffer ? nm::float4(tangentsBuffer[(v * 4) + 0], tangentsBuffer[(v * 4) + 1], tangentsBuffer[(v * 4) + 2], tangentsBuffer[(v * 4) + 3]) : nm::float4(0.0))[0]);
						if (p_loadData.flipUV == true)
						{
							float* uv = vert.GetAttribute(Vertex::AttributeFlag::uv);
							uv[1] = 1.0f - uv[1];
						}

						float* position = vert.GetAttribute(Vertex::AttributeFlag::position);
						bbMin[0] = std::min(bbMin[0], position[0]);
						bbMin[1] = std::min(bbMin[1], position[1]);
						bbMin[2] = std::min(bbMin[2], position[2]);

						bbMax[0] = std::max(bbMax[0], position[0]);
						bbMax[1] = std::max(bbMax[1], position[1]);
						bbMax[2] = std::max(bbMax[2], position[2]);

						objMesh.vertexList.AddVertex(vert);
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

				SubMesh submesh{};
				submesh.name		= mesh.name + "_" + std::to_string(objMesh.submeshes.size());	// this is to make sure all names are unique
				submesh.firstIndex	= firstIndex;
				submesh.indexCount	= indexCount;
				submesh.materialId	= material_offset + glTFPrimitive.material;
				objMesh.submeshes.push_back(submesh);

				BBox meshbox(BBox::Type::Custom, BBox::Origin::Center, bbMin, bbMax);
				ComputeBBox(meshbox);
				objMesh.submeshesBbox.push_back(meshbox);
				
			}
		}
	}

	nm::float3 bbMin = nm::float3{ 0.0f, 0.0f, 0.0f };
	nm::float3 bbMax = nm::float3{ 0.0f, 0.0f, 0.0f };
	for(int i = 0; i < objMesh.submeshesBbox.size(); i++)
	{		
		bbMin[0] = std::min(bbMin[0], objMesh.submeshesBbox[i].bbMin[0]);
		bbMin[1] = std::min(bbMin[1], objMesh.submeshesBbox[i].bbMin[1]);
		bbMin[2] = std::min(bbMin[2], objMesh.submeshesBbox[i].bbMin[2]);
															 
		bbMax[0] = std::max(bbMax[0], objMesh.submeshesBbox[i].bbMax[0]);
		bbMax[1] = std::max(bbMax[1], objMesh.submeshesBbox[i].bbMax[1]);
		bbMax[2] = std::max(bbMax[2], objMesh.submeshesBbox[i].bbMax[2]);
	}	

	objMesh.bbox = BBox(BBox::Type::Custom, BBox::Origin::Center, bbMin, bbMax);
	BBox* meshBBox = &objMesh.bbox;
	ComputeBBox(*meshBBox);

	
	{
		// correcting the translation of the mesh based on its min and max 
		nm::float3 translationFactor = -(objMesh.bbox.bbMin + objMesh.bbox.bbMax) / 2.0f;
		objMesh.transform = nm::translation(translationFactor);
		
	}

	p_objScene.meshList.push_back(objMesh);

	// load materials
	for (auto& gltf_mat : input.materials)
	{
		Material mat;
		if (gltf_mat.values.find("baseColorTexture") != gltf_mat.values.end())
		{
			mat.color_id = texture_offset + gltf_mat.values["baseColorTexture"].TextureIndex();
		}

		if (gltf_mat.additionalValues.find("normalTexture") != gltf_mat.additionalValues.end())
		{
			mat.normal_id = texture_offset + gltf_mat.additionalValues["normalTexture"].TextureIndex();
		}
		
		mat.pbr_color		= nm::float3(1.0f); // nm::float3((float)gltf_mat.pbrMetallicRoughness.baseColorFactor[0], (float)gltf_mat.pbrMetallicRoughness.baseColorFactor[1], (float)gltf_mat.pbrMetallicRoughness.baseColorFactor[2]);
		mat.metallic		= (float)gltf_mat.pbrMetallicRoughness.metallicFactor;
		mat.roughness		= (float)gltf_mat.pbrMetallicRoughness.roughnessFactor;
		mat.roughMetal_id	= texture_offset + ((gltf_mat.pbrMetallicRoughness.metallicRoughnessTexture.index < 0) ? MAX_SUPPORTED_TEXTURES : gltf_mat.pbrMetallicRoughness.metallicRoughnessTexture.index);

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
	objMesh.vertexList	= VertexList(Vertex::AttributeFlag::position | Vertex::AttributeFlag::normal | Vertex::AttributeFlag::uv | Vertex::AttributeFlag::tangent);
	
	auto vertList		= &objMesh.vertexList;
	auto indList		= &objMesh.indicesList;

	float emptyUV[2]	= {0.0f, 0.0f};

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	int meshCount = 0;
	for (const auto& shape : shapes)
	{
		{
			int i = 0;
			int valid_i = 0;
			for (const auto& index : shape.mesh.indices)
			{
				Vertex vertex = objMesh.vertexList.CreateVertex();
				vertex.AddAttribute(Vertex::AttributeFlag::position, &attrib.vertices[3 * index.vertex_index]);
				vertex.AddAttribute(Vertex::AttributeFlag::normal, &attrib.normals[3 * index.normal_index]);

				int in = index.texcoord_index;
				if (in < 0)
					in = 0;

				if (attrib.texcoords.empty())
					vertex.AddAttribute(Vertex::AttributeFlag::uv, &emptyUV[0]);
				else
					vertex.AddAttribute(Vertex::AttributeFlag::uv, &attrib.texcoords[2 * in + 0]);

				if (p_loadData.flipUV == true)
				{
					float* uv = vertex.GetAttribute(Vertex::AttributeFlag::uv);
					uv[1] = 1.0f - uv[1];
				}

				//vertex.color = nm::float3{1.0f, 1.0f, 1.0f
					/*attrib.colors[3 * index.vertex_index + 0],
					attrib.colors[3 * index.vertex_index + 1],
					attrib.colors[3 * index.vertex_index + 2]*/
				//};

				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(vertList->size() / vertList->GetVertexSize());
					vertList->AddVertex(vertex);
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
			ImageRaw diffuse{ nullptr, 0, 0, 0 };		// initializing diffuse raw texture
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
			ImageRaw normal{ nullptr, 0, 0, 0 };	// initialized normal raw texture
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
				std::cout << "Normal Mat Not Found: " << material.name << std::endl;
			}

			p_objScene.textureList.push_back(normal);
		}
	}

	return true;
}

bool WriteToDisk(const std::filesystem::path& pPath, size_t pDataSize, char* pData)
{
	std::ofstream myfile(pPath.string().c_str(), std::ios::out | std::ios::binary);
	if (myfile.is_open())
	{
		std::cout << "Writing to disk successful: " << pPath << std::endl;
		myfile.write(pData, pDataSize);
		myfile.close();
		return true;
	}
	std::cout << "Writing to disk failed: " << pPath << std::endl;
	return false;		
}

std::vector<uint32_t> BBox::GetIndexTemplate()
{
	std::vector<uint32_t> indexTemplate{ 0, 1, 2, 3, 4, 5, 6, 7, 0, 4, 1, 5, 2, 6, 3, 7, 1, 2, 3, 0, 5, 6, 7, 4 };
	return indexTemplate;
}

VertexList BBox::GetVertexTempate()
{
	VertexList vertices(Vertex::AttributeFlag::position);
	vertices.AddVertex(Vertex::AttributeFlag::position, &nm::float3{ -1, -1, -1 } [0] );
	vertices.AddVertex(Vertex::AttributeFlag::position, &nm::float3{  1, -1, -1 } [0] );
	vertices.AddVertex(Vertex::AttributeFlag::position, &nm::float3{  1,  1, -1 } [0] );
	vertices.AddVertex(Vertex::AttributeFlag::position, &nm::float3{ -1,  1, -1 } [0] );
	vertices.AddVertex(Vertex::AttributeFlag::position, &nm::float3{ -1, -1, 1  } [0] );
	vertices.AddVertex(Vertex::AttributeFlag::position, &nm::float3{  1, -1, 1  } [0] );
	vertices.AddVertex(Vertex::AttributeFlag::position, &nm::float3{  1,  1, 1  } [0] );
	vertices.AddVertex(Vertex::AttributeFlag::position, &nm::float3{ -1,  1, 1  } [0] );
	return vertices;
}

BBox::BBox(Type p_type, Origin p_origin, nm::float3 p_min, nm::float3 p_max)
	:unitBBoxTransform()
{
	Reset(p_type, p_origin, p_min, p_max);
}

BBox BBox::operator*(nm::float4x4 const& p_transform)
{
	BBox box;

	box.bbMin = (p_transform * nm::float4(this->bbMin, 1.0)).xyz();
	box.bbMax = (p_transform * nm::float4(this->bbMax, 1.0)).xyz();

	box.CalculateCorners();
	box.CalculateUnitBBoxTransform();

	return box;
}

BBox BBox::operator*(nm::Transform const& p_transform)
{
	BBox box;
	if (!(p_transform.GetRotate() == nm::float4x4().identity()))
	{
		for (int i = 0; i < 8; i++)
		{
			box.bBox[i] = (p_transform.GetTransform() * nm::float4(this->bBox[i], 1.0)).xyz();

			box.bbMin[0] = (box.bbMin[0] <= box.bBox[i][0]) ? box.bbMin[0] : box.bBox[i][0];
			box.bbMin[1] = (box.bbMin[1] <= box.bBox[i][1]) ? box.bbMin[1] : box.bBox[i][1];
			box.bbMin[2] = (box.bbMin[2] <= box.bBox[i][2]) ? box.bbMin[2] : box.bBox[i][2];

			box.bbMax[0] = (box.bbMax[0] > box.bBox[i][0]) ? box.bbMax[0] : box.bBox[i][0];
			box.bbMax[1] = (box.bbMax[1] > box.bBox[i][1]) ? box.bbMax[1] : box.bBox[i][1];
			box.bbMax[2] = (box.bbMax[2] > box.bBox[i][2]) ? box.bbMax[2] : box.bBox[i][2];
		}
	}
	else
	{
		box.bbMin = (p_transform.GetTransform()  * nm::float4(this->bbMin, 1.0)).xyz();
		box.bbMax = (p_transform.GetTransform() * nm::float4(this->bbMax, 1.0)).xyz();

		box.CalculateCorners();
		box.CalculateUnitBBoxTransform();
	}

	return box;
}

bool BBox::operator==(const BBox& other)
{
	if (this->bbMin == other.bbMin && this->bbMax == other.bbMax)
		return true;
	
	return false;
}

// merge this with that
void BBox::Merge(const BBox& that)
{
	this->bbMin[0] = (this->bbMin[0] <= that.bbMin[0]) ? this->bbMin[0] : that.bbMin[0];
	this->bbMin[1] = (this->bbMin[1] <= that.bbMin[1]) ? this->bbMin[1] : that.bbMin[1];
	this->bbMin[2] = (this->bbMin[2] <= that.bbMin[2]) ? this->bbMin[2] : that.bbMin[2];

	this->bbMax[0] = (this->bbMax[0] > that.bbMax[0]) ? this->bbMax[0] : that.bbMax[0];
	this->bbMax[1] = (this->bbMax[1] > that.bbMax[1]) ? this->bbMax[1] : that.bbMax[1];
	this->bbMax[2] = (this->bbMax[2] > that.bbMax[2]) ? this->bbMax[2] : that.bbMax[2];

	CalculateCorners();
	CalculateUnitBBoxTransform();
}

// merge this with that
void BBox::Merge(const BSphere& that)
{
	this->bbMin[0] = (this->bbMin[0] <= (that.bsCenter[0] - that.bsRadius)) ? this->bbMin[0] : (that.bsCenter[0] - that.bsRadius);
	this->bbMin[1] = (this->bbMin[1] <= (that.bsCenter[1] - that.bsRadius)) ? this->bbMin[1] : (that.bsCenter[1] - that.bsRadius);
	this->bbMin[2] = (this->bbMin[2] <= (that.bsCenter[2] - that.bsRadius)) ? this->bbMin[2] : (that.bsCenter[2] - that.bsRadius);

	this->bbMax[0] = (this->bbMax[0] > (that.bsCenter[0] + that.bsRadius)) ? this->bbMax[0] : (that.bsCenter[0] + that.bsRadius);
	this->bbMax[1] = (this->bbMax[1] > (that.bsCenter[1] + that.bsRadius)) ? this->bbMax[1] : (that.bsCenter[1] + that.bsRadius);
	this->bbMax[2] = (this->bbMax[2] > (that.bsCenter[2] + that.bsRadius)) ? this->bbMax[2] : (that.bsCenter[2] + that.bsRadius);

	CalculateCorners();
	CalculateUnitBBoxTransform();
}

void BBox::Reset(Type p_type, Origin p_origin, nm::float3 p_min, nm::float3 p_max)
{
	bbMin = nm::float3{ 0.0f };
	bbMax = nm::float3{ 0.0f };

	if (p_type == Unit)
	{
		bbMin = nm::float3{ -1.0f, -1.0f, -1.0f };
		bbMax = nm::float3{ 1.0f, 1.0f, 1.0f };
	}

	if (p_origin == CameraStyle)
	{
		bbMin = nm::float3{ -1.0f, -1.0f, 0.0f };
		bbMax = nm::float3{ 1.0f, 1.0f, 2.0f };
	}

	if (p_type == Custom)
	{
		bbMin = p_min;
		bbMax = p_max;
	}

	CalculateCorners();
	CalculateUnitBBoxTransform();
}

float BBox::GetDepth() const
{
	return std::abs(bbMin[2]) + std::abs(bbMax[2]);
}

float BBox::GetHeight() const
{
	return std::abs(bbMin[1]) + std::abs(bbMax[1]);
}

float BBox::GetWidth() const
{
	return std::abs(bbMin[0]) + std::abs(bbMax[0]);
}

bool BBox::isVisiable(BBox p_b)
{
	for (int i = 0; i < 8; i++)
	{
		if (p_b.bBox[i].x() > this->bbMin.x() && p_b.bBox[i].x() <= this->bbMax.x() &&
			p_b.bBox[i].x() > this->bbMin.y() && p_b.bBox[i].y() <= this->bbMax.y() &&
			p_b.bBox[i].x() > this->bbMin.z() && p_b.bBox[i].z() <= this->bbMax.z())
		{
			return true;
		}
	}
	return false;
}

bool BBox::isVisiable(BSphere p_s)
{
	for (int i = 0; i < 8; i++)
	{
		if (nm::distance(p_s.bsCenter, this->bBox[i]) <= p_s.bsRadius)
		{
			return true;
		}
	}
	return false;
}

nm::Transform BBox::GetUnitBBoxTransform() const
{
	return unitBBoxTransform;
}

void BBox::CalculateUnitBBoxTransform()
{
	// This function assumes the bounding-box to be of unit size
	// and calculates scaling and translation factor of this
	// box with min and max values. 
	nm::float4x4 scale = nm::float4x4::identity();
	scale.column[0][0] = (bbMax.x() - bbMin.x())/2.0f;
	scale.column[1][1] = (bbMax.y() - bbMin.y())/2.0f;
	scale.column[2][2] = (bbMax.z() - bbMin.z())/2.0f;
	scale.column[3][3] = 1.0f;
	unitBBoxTransform.SetScale(scale);
	
	nm::float4 translate = (nm::float4(bbMax, 1.0f) + nm::float4(bbMin, 1.0f))/2.0f;
	unitBBoxTransform.SetTranslate(translate);
}

void BBox::CalculateCorners()
{
	bBox[0] = nm::float3{ bbMin[0], bbMin[1], bbMin[2] };
	bBox[1] = nm::float3{ bbMax[0], bbMin[1], bbMin[2] };
	bBox[2] = nm::float3{ bbMax[0], bbMax[1], bbMin[2] };
	bBox[3] = nm::float3{ bbMin[0], bbMax[1], bbMin[2] };
	bBox[4] = nm::float3{ bbMin[0], bbMin[1], bbMax[2] };
	bBox[5] = nm::float3{ bbMax[0], bbMin[1], bbMax[2] };
	bBox[6] = nm::float3{ bbMax[0], bbMax[1], bbMax[2] };
	bBox[7] = nm::float3{ bbMin[0], bbMax[1], bbMax[2] };
}

BSphere::BSphere()
{
	bsCenter = nm::float3(0.0f, 0.0f, 0.0f);
	bsRadius = 1.0f;
}

BSphere BSphere::operator*(nm::Transform const& p_transform)
{
	BSphere bSphere;
	if (!(p_transform.GetRotate() == nm::float4x4().identity()))
	{
		bSphere.bsCenter = const_cast<nm::Transform*>(&p_transform)->GetTranslateVector();
	}

	bsBox = bsBox * p_transform;

	return bSphere;
}

bool BSphere::operator==(const BSphere& other)
{
	if (this->bsCenter == other.bsCenter && this->bsRadius == other.bsRadius)
		return true;

	return false;
}

bool BSphere::isVisiable(BBox p_b)
{
	for (int i = 0; i < 8; i++)
	{
		if (nm::distance(this->bsCenter, p_b.bBox[i]) <= this->bsRadius)
		{
			return true;
		}
	}
	return false;
}

bool BSphere::isVisiable(BSphere p_s)
{
	if (nm::distance(this->bsCenter, p_s.bsCenter) <= (this->bsRadius + p_s.bsRadius))
	{
		return true;
	}
	return true;
}
