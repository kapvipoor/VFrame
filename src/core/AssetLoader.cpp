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
	//p_bbox.bBox[0] = nm::float3{ p_bbox.bbMin[0], p_bbox.bbMin[1], p_bbox.bbMin[2] };
	//p_bbox.bBox[1] = nm::float3{ p_bbox.bbMax[0], p_bbox.bbMin[1], p_bbox.bbMin[2] };
	//p_bbox.bBox[2] = nm::float3{ p_bbox.bbMax[0], p_bbox.bbMax[1], p_bbox.bbMin[2] };
	//p_bbox.bBox[3] = nm::float3{ p_bbox.bbMin[0], p_bbox.bbMax[1], p_bbox.bbMin[2] };
	//p_bbox.bBox[4] = nm::float3{ p_bbox.bbMin[0], p_bbox.bbMin[1], p_bbox.bbMax[2] };
	//p_bbox.bBox[5] = nm::float3{ p_bbox.bbMax[0], p_bbox.bbMin[1], p_bbox.bbMax[2] };
	//p_bbox.bBox[6] = nm::float3{ p_bbox.bbMax[0], p_bbox.bbMax[1], p_bbox.bbMax[2] };
	//p_bbox.bBox[7] = nm::float3{ p_bbox.bbMin[0], p_bbox.bbMax[1], p_bbox.bbMax[2] };
}

// Heavily inspired from - http://www.songho.ca/opengl/gl_sphere.html
void GenerateSphere(int p_stackCount, int p_sectorCount, RawSphere& p_sphere, float p_radius)
{
	p_sphere.indices.clear();
	p_sphere.lineIndices.clear();

	float x, y, z, xy;									// vertex position
	
	float sectorStep = 2 * nm::PI / p_sectorCount;
	float stackStep = nm::PI / p_stackCount;
	float sectorAngle, stackAngle;

	for (int i = 0; i <= p_stackCount; ++i)
	{
		stackAngle = nm::PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
		xy = p_radius * cosf(stackAngle);             // r * cos(u)
		z = p_radius * sinf(stackAngle);              // r * sin(u)

		// add (sectorCount+1) vertices per stack
		// the first and last vertices have same position and normal, but different texture coordinates
		for (int j = 0; j <= p_sectorCount; ++j)
		{
			sectorAngle = j * sectorStep;           // starting from 0 to 2pi

			// vertex position (x, y, z)
			x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
			y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
			//p_sphere.vertices.push_back(nm::float3(x, y, z));
			p_sphere.vertices.AddVertex(Vertex::AttributeFlag::position, &nm::float3{ x, y, z }[0]);
		}
	}

	// indices
	//  k1--k1+1
	//  |  / |
	//  | /  |
	//  k2--k2+1
	unsigned int k1, k2;
	for (int i = 0; i < p_stackCount; ++i)
	{
		k1 = i * (p_sectorCount + 1);     // beginning of current stack
		k2 = k1 + p_sectorCount + 1;      // beginning of next stack

		for (int j = 0; j < p_sectorCount; ++j, ++k1, ++k2)
		{
			// 2 triangles per sector excluding 1st and last stacks
			if (i != 0)
			{
				p_sphere.indices.push_back(k1);
				p_sphere.indices.push_back(k2);
				p_sphere.indices.push_back(k1 + 1);
				//addIndices(k1, k2, k1 + 1);   // k1---k2---k1+1
			}

			if (i != (p_stackCount - 1))
			{
				p_sphere.indices.push_back(k1 + 1);
				p_sphere.indices.push_back(k2);
				p_sphere.indices.push_back(k2 + 1);
				//addIndices(k1 + 1, k2, k2 + 1); // k1+1---k2---k2+1
			}

			// vertical lines for all stacks
			p_sphere.lineIndices.push_back(k1);
			p_sphere.lineIndices.push_back(k2);
			if (i != 0)  // horizontal lines except 1st stack
			{
				p_sphere.lineIndices.push_back(k1);
				p_sphere.lineIndices.push_back(k1 + 1);
			}
		}
	}
}

bool LoadRawImage(const char* p_path, ImageRaw& p_data)
{
	GetFileName(p_path, p_data.name);

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
	free(p_data.raw);
	p_data = ImageRaw{};
}

bool LoadMaterials(tinygltf::Model p_gltfInput, SceneRaw& p_objScene, uint32_t p_texOffset)
{
	// load materials
	int materialCount = 0;
	for (auto& gltf_mat : p_gltfInput.materials)
	{
		std::cout << "Loading Material: " << materialCount << " of " << p_gltfInput.materials.size() << std::endl;

		Material mat;
		if (gltf_mat.values.find("baseColorTexture") != gltf_mat.values.end())
		{
			mat.color_id = p_texOffset + gltf_mat.values["baseColorTexture"].TextureIndex();
		}

		if (gltf_mat.additionalValues.find("normalTexture") != gltf_mat.additionalValues.end())
		{
			mat.normal_id = p_texOffset + gltf_mat.additionalValues["normalTexture"].TextureIndex();
		}

		mat.pbr_color = nm::float3(1.0f); // nm::float3((float)gltf_mat.pbrMetallicRoughness.baseColorFactor[0], (float)gltf_mat.pbrMetallicRoughness.baseColorFactor[1], (float)gltf_mat.pbrMetallicRoughness.baseColorFactor[2]);
		mat.metallic = (float)gltf_mat.pbrMetallicRoughness.metallicFactor;
		mat.roughness = (float)gltf_mat.pbrMetallicRoughness.roughnessFactor;
		mat.roughMetal_id = p_texOffset + ((gltf_mat.pbrMetallicRoughness.metallicRoughnessTexture.index < 0) ? MAX_SUPPORTED_TEXTURES : gltf_mat.pbrMetallicRoughness.metallicRoughnessTexture.index);

		p_objScene.materialsList.push_back(mat);
		materialCount++;
	}

	return true;
}

bool LoadTextures(tinygltf::Model p_gltfInput, SceneRaw& p_objScene, std::string p_folder)
{
	int textureCount = 0;
	for (const auto& image : p_gltfInput.images)
	{
		std::cout << "Loading Texture: " << textureCount << " of " << p_gltfInput.images.size() << std::endl;

		ImageRaw iraw{ "", nullptr, 0, 0, 0 };
		std::string path = (p_folder + "/" + image.uri);

		if (image.image.empty())
		{
			iraw.raw = stbi_load(path.c_str(), &iraw.width, &iraw.height, &iraw.channels, STBI_rgb_alpha);
			iraw.channels = STBI_rgb_alpha;

			if (iraw.raw == nullptr)
			{
				std::cerr << "stbi_load Failed - " << path << std::endl;
				return false;
			}
		}
		else
		{
			unsigned char* test = nullptr;
			iraw.name = image.uri;
			iraw.width = image.width;
			iraw.height = image.height;
			iraw.channels = image.component;

			iraw.raw = static_cast<unsigned char*>(malloc(image.image.size()));
			memcpy(iraw.raw, image.image.data(), image.image.size());
		}		
		p_objScene.textureList.push_back(iraw);
		
		textureCount++;
	}

	if (p_gltfInput.images.empty())
	{
		ImageRaw iraw{ "", nullptr, 0, 0, 0};
		p_objScene.textureList.push_back(iraw);
	}

	return true;
}

bool LoadNode(const tinygltf::Node node, tinygltf::Model input, MeshRaw& objMesh, const ObjLoadData& p_loadData, uint32_t p_matOffset, nm::float4x4 transform)
{
	nm::float4x4 temp_transform = transform;
	if (node.translation.size() == 3) {
		temp_transform = temp_transform * nm::translation(nm::float3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]));
	}
	if (node.rotation.size() == 4) {
		nm::quatf  q((float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2], (float)node.rotation[3]);
		nm::float4x4 q_mat = nm::transpose(nm::quat2mat_glm(q));
		temp_transform = temp_transform * q_mat;
	}
	if (node.scale.size() == 3) {
		temp_transform = temp_transform * nm::scale(nm::float4((float)node.scale[0], (float)node.scale[1], (float)node.scale[2], 1.0));
	}

	// Load node's children
	if (node.children.size() > 0) {
		for (size_t i = 0; i < node.children.size(); i++) {
			LoadNode(input.nodes[node.children[i]], input, objMesh, p_loadData, p_matOffset, temp_transform);
		}
	}

	if (node.mesh > -1)
	{
		const tinygltf::Mesh mesh = input.meshes[node.mesh];
		for (size_t i = 0; i < mesh.primitives.size(); i++)
		{
			const tinygltf::Primitive& glTFPrimitive = mesh.primitives[i];
			uint32_t firstIndex = static_cast<uint32_t>(objMesh.indicesList.size());
			uint32_t vertexStart = static_cast<uint32_t>(objMesh.vertexList.size() / objMesh.vertexList.GetVertexSize());
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
					vert.AddAttribute(Vertex::AttributeFlag::position, &((temp_transform * 
						nm::float4(positionBuffer[(v * 3) + 0], positionBuffer[(v * 3) + 1], positionBuffer[(v * 3) + 2], 1.0f)).xyz())[0]);
					
					vert.AddAttribute(Vertex::AttributeFlag::normal, 
						&(normalsBuffer ? 
							nm::normalize(nm::float3(normalsBuffer[(v * 3) + 0], normalsBuffer[(v * 3) + 1], normalsBuffer[(v * 3) + 2])) : 
							nm::float3(0.0f))[0]);
					
					vert.AddAttribute(Vertex::AttributeFlag::uv, 
						&(texCoordsBuffer ? 
							nm::float2(texCoordsBuffer[(v * 2) + 0], texCoordsBuffer[(v * 2) + 1]) : 
							nm::float2(0.0f))[0]);
					
					vert.AddAttribute(Vertex::AttributeFlag::tangent, 
						&(tangentsBuffer ? 
							nm::float4(tangentsBuffer[(v * 4) + 0], tangentsBuffer[(v * 4) + 1], tangentsBuffer[(v * 4) + 2], tangentsBuffer[(v * 4) + 3]) : 
							nm::float4(0.0))[0]);

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
			submesh.name = mesh.name + "_" + std::to_string(objMesh.submeshes.size());	// this is to make sure all names are unique
			submesh.firstIndex = firstIndex;
			submesh.indexCount = indexCount;
			submesh.materialId = p_matOffset + glTFPrimitive.material;
			objMesh.submeshes.push_back(submesh);

			BBox meshbox(BBox::Type::Custom, BBox::Origin::Center, bbMin, bbMax);
			objMesh.submeshesBbox.push_back(meshbox);

		}
	}

	return true;
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
		found = strPath.find_last_of("\\");
		if (found == std::string::npos)
		{
			std::cerr << "Invalid gltf path - " << p_path << std::endl;
			return false;
		}
	}
	std::string folderPath = strPath.substr(0, found);

	std::string fileExtn;
	if(!GetFileExtention(p_path, fileExtn))
	{
		std::cout << "Failed to Get Extn - " << p_path << std::endl;
		return false;
	}

	if (fileExtn == "gltf")
	{
		if (!gltfContext.LoadASCIIFromFile(&input, &error, &warning, p_path))
		{
			std::cerr << "Failed to load Gltf - " << p_path << std::endl;
			std::cerr << error << std::endl;
			return false;
		}
	}
	else if (fileExtn == "glb")
	{
		if (!gltfContext.LoadBinaryFromFile(&input, &error, &warning, p_path))
		{
			std::cerr << "Failed to load Gltf - " << p_path << std::endl;
			std::cerr << error << std::endl;
			return false;
			return false;
		}
	}

	//uint32_t material_offset = (uint32_t)p_objScene.materialsList.size();
	//uint32_t texture_offset = (uint32_t)p_objScene.textureList.size();

	RETURN_FALSE_IF_FALSE(LoadMaterials(input, p_objScene, p_objScene.textureOffset));
	RETURN_FALSE_IF_FALSE(LoadTextures(input, p_objScene, folderPath));
			
	MeshRaw objMesh;
	objMesh.vertexList = VertexList(Vertex::AttributeFlag::position | Vertex::AttributeFlag::normal | Vertex::AttributeFlag::uv | Vertex::AttributeFlag::tangent);
	if (!GetFileName(p_path, objMesh.name, "/"))
	{
		if (!GetFileName(p_path, objMesh.name, "\\"))
		{
			std::cerr << "Failed to Get Filename - " << p_path << std::endl;
			return false;
		}
	}

	const tinygltf::Scene& scene = input.scenes[0];
	for (size_t n_id = 0; n_id < scene.nodes.size(); n_id++)
	{
		std::cout << "Loading GLTF Node: " << n_id << " of " << scene.nodes.size() << std::endl;
		const tinygltf::Node node = input.nodes[n_id];
		LoadNode(node, input, objMesh, p_loadData, p_objScene.materialOffset, nm::float4x4::identity());
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
	//ComputeBBox(*meshBBox);
	{
		// correcting the translation of the mesh based on its min and max 
		//nm::float3 translationFactor = -(objMesh.bbox.bbMin + objMesh.bbox.bbMax) / 2.0f;
		//objMesh.transform.SetTranslate((nm::translation(translationFactor)));
	}

	p_objScene.meshList.push_back(objMesh);

	p_objScene.materialOffset = (uint32_t)p_objScene.materialsList.size();
	p_objScene.textureOffset = (uint32_t)p_objScene.textureList.size();

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
			ImageRaw diffuse{ "", nullptr, 0, 0, 0};		// initializing diffuse raw texture
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
			ImageRaw normal{ "", nullptr, 0, 0, 0};	// initialized normal raw texture
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
	: BVolume(BVolume::BType::Box)
	, unitBBoxTransform()
{
	Reset(p_type, p_origin, p_min, p_max);
}

// Note that we do not directly multiply the incoming transform with the unit
// box transform. Because the unit box transform cannot be used to directly
// multiply to. Its used a representation of transform required to render the
// bounding box that ranges (-1,1) along x, y and z by default. The unit box
// transform is multiplied to  this unit box to generate box of its logical
// representation. Hence, the transform is multiplied to the min and mix of the
// bounding box and a new min max generated (if there is also rotation). from
// those values, a new unit box transform is computed.
BBox BBox::operator*(nm::Transform const& p_transform)
{
	BBox box;
	
	std::vector<nm::float3> corners = CalculateCorners(this->bbMin, this->bbMax);
	CalculateMinMaxfromCorners(corners, box.bbMin, box.bbMax, p_transform);
	
	box.unitBBoxTransform.SetTranslate(CalculateTranslate(box.bbMin, box.bbMax));
	box.unitBBoxTransform.SetScale(CalculateScale(box.bbMin, box.bbMax));
	
	return box;
}

BBox BBox::operator*(nm::float4x4 const& p_transform)
{
	BBox box;

	std::vector<nm::float3> corners = CalculateCorners(this->bbMin, this->bbMax);
	CalculateMinMaxfromCorners(corners, box.bbMin, box.bbMax, p_transform);

	box.unitBBoxTransform.SetTranslate(CalculateTranslate(box.bbMin, box.bbMax));
	box.unitBBoxTransform.SetScale(CalculateScale(box.bbMin, box.bbMax));

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

	unitBBoxTransform.SetTranslate(CalculateTranslate(bbMin, bbMax));
	unitBBoxTransform.SetScale(CalculateScale(bbMin, bbMax));
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

	unitBBoxTransform.SetScale(CalculateScale(bbMin, bbMax));
	
}

void BBox::Reset(Type p_type, Origin p_origin, nm::float3 p_min, nm::float3 p_max)
{
	origin = p_origin;
	bbMin = nm::float3{ 0.0f };
	bbMax = nm::float3{ 0.0f };

	if (p_type == Unit)
	{
		bbMin = nm::float3{ -1.0f, -1.0f, -1.0f };
		bbMax = nm::float3{ 1.0f, 1.0f, 1.0f };
		unitBBoxTransform = nm::Transform();
	}

	if (p_origin == CameraStyle)
	{
		bbMin = nm::float3{ -1.0f, -1.0f, -1.0f };
		bbMax = nm::float3{ 1.0f, 1.0f, 1.0f };
		unitBBoxTransform = nm::Transform();
	}

	if (p_type == Custom)
	{
		bbMin = p_min;
		bbMax = p_max;
		nm::float4x4 translate = CalculateTranslate(p_min, p_max);
		nm::float4x4 rotate = nm::float4x4::identity();
		nm::float4x4 scale = CalculateScale(p_min, p_max);
		unitBBoxTransform = nm::Transform(translate, rotate, scale);
	}
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
	//for (int i = 0; i < 8; i++)
	//{
	//	if (p_b.bBox[i].x() > this->bbMin.x() && p_b.bBox[i].x() <= this->bbMax.x() &&
	//		p_b.bBox[i].x() > this->bbMin.y() && p_b.bBox[i].y() <= this->bbMax.y() &&
	//		p_b.bBox[i].x() > this->bbMin.z() && p_b.bBox[i].z() <= this->bbMax.z())
	//	{
	//		return true;
	//	}
	//}
	return false;
}

bool BBox::isVisiable(BSphere p_s)
{
	//for (int i = 0; i < 8; i++)
	//{
	//	if (nm::distance(p_s.bsCenter, this->bBox[i]) <= p_s.bsRadius)
	//	{
	//		return true;
	//	}
	//}
	return false;
}

nm::Transform BBox::GetUnitBBoxTransform() const
{
	return unitBBoxTransform;
}

//void BBox::SetUnitBoxTranform(nm::float4x4 p_Transform)
//{
//	std::vector<nm::float3> corners = CalculateCorners(bbMin, bbMax);
//	CalculateMinMaxfromCorners(corners, bbMin, bbMax, nm::Transform(p_Transform));
//	nm::float4x4 offsetTransform = nm::float4x4::identity();
//	if (origin == Origin::CameraStyle)
//	{
//		offsetTransform.column[2][2] = 0.5f;
//		unitBBoxTransform = nm::Transform(p_Transform * offsetTransform);
//		
//		nm::float4x4 withTranslation = unitBBoxTransform.GetTransform();
//		withTranslation.column[3][2] -= (bbMax[2])/3.0f;
//		unitBBoxTransform = nm::Transform(withTranslation);
//	}
//	else
//	{
//		unitBBoxTransform = nm::Transform(p_Transform * offsetTransform);
//	}
//}

std::vector<nm::float3> BBox::CalculateCorners(nm::float3 p_bbMin, nm::float3 p_bbMax)
{
	std::vector<nm::float3> bBox(8, nm::float3());
	bBox[0] = nm::float3{ p_bbMin[0], p_bbMin[1], p_bbMin[2] };
	bBox[1] = nm::float3{ p_bbMax[0], p_bbMin[1], p_bbMin[2] };
	bBox[2] = nm::float3{ p_bbMax[0], p_bbMax[1], p_bbMin[2] };
	bBox[3] = nm::float3{ p_bbMin[0], p_bbMax[1], p_bbMin[2] };
	bBox[4] = nm::float3{ p_bbMin[0], p_bbMin[1], p_bbMax[2] };
	bBox[5] = nm::float3{ p_bbMax[0], p_bbMin[1], p_bbMax[2] };
	bBox[6] = nm::float3{ p_bbMax[0], p_bbMax[1], p_bbMax[2] };
	bBox[7] = nm::float3{ p_bbMin[0], p_bbMax[1], p_bbMax[2] };
	return bBox;
}

void BBox::CalculateMinMaxfromCorners(std::vector<nm::float3> p_corners, nm::float3& p_min, nm::float3& p_max, nm::Transform p_transform)
{
	p_min = nm::float3{};
	p_max = nm::float3{};

	if (p_corners.empty())
	{
		std::cerr << "BBox::CalculateMinMaxfromCorners p_corners is empty" << std::endl;
		return;
	}

	if (p_corners.size() != 8)
	{
		std::cerr << "BBox::CalculateMinMaxfromCorners p_corners != 8" << std::endl;
		return;
	}

	//for (auto& corner : p_corners)
	for (int i = 0; i < 8; i++)
	{
		nm::float3 corner = (p_transform.GetTransform() * nm::float4(p_corners[i], 1.0)).xyz();
		if (i == 0)
		{
			p_min = corner;
			p_max = corner;
		}

		p_min[0] = (p_min[0] <= corner[0]) ? p_min[0] : corner[0];
		p_min[1] = (p_min[1] <= corner[1]) ? p_min[1] : corner[1];
		p_min[2] = (p_min[2] <= corner[2]) ? p_min[2] : corner[2];

		p_max[0] = (p_max[0] > corner[0]) ? p_max[0] : corner[0];
		p_max[1] = (p_max[1] > corner[1]) ? p_max[1] : corner[1];
		p_max[2] = (p_max[2] > corner[2]) ? p_max[2] : corner[2];
	}
}

nm::float4x4 BBox::CalculateScale(nm::float3 p_min, nm::float3 p_max)
{
	nm::float4x4 scale = nm::float4x4::identity();
	scale.column[0][0] = (p_max.x() - p_min.x())/2.0f;
	scale.column[1][1] = (p_max.y() - p_min.y())/2.0f;
	scale.column[2][2] = (p_max.z() - p_min.z())/2.0f;
	scale.column[3][3] = 1.0f;
	return scale;
}

nm::float4x4 BBox::CalculateTranslate(nm::float3 p_min, nm::float3 p_max)
{
	nm::float4x4 transate = nm::float4x4::identity();
	nm::float4 translateVec = (nm::float4(p_min, 1.0f) + nm::float4(p_max, 1.0f)) / 2.0f;
	transate.column[3] = translateVec;
	return transate;
}

BSphere::BSphere()
	: BVolume(BVolume::BType::Sphere)
{
	bsCenter = nm::float3(0.0f, 0.0f, 0.0f);
	bsRadius = 1.0f;
}

bool BSphere::operator==(const BSphere& other)
{
	if (this->bsCenter == other.bsCenter && this->bsRadius == other.bsRadius)
		return true;

	return false;
}

bool BSphere::isVisiable(BBox p_b)
{
	//for (int i = 0; i < 8; i++)
	//{
	//	if (nm::distance(this->bsCenter, p_b.bBox[i]) <= this->bsRadius)
	//	{
	//		return true;
	//	}
	//}
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

std::vector<int> BFrustum::GetIndexTemplate()
{
	std::vector<int> indexTemplate{ 0, 1, 2, 3, 4, 5, 6, 7, 0, 4, 1, 5, 2, 6, 3, 7, 1, 2, 3, 0, 5, 6, 7, 4 };
	return indexTemplate;
}

VertexList BFrustum::GetVertexTempate()
{
	VertexList vertices(Vertex::AttributeFlag::position);
	vertices.AddVertex(Vertex::AttributeFlag::position, &nm::float3{ -1.0f,		-1.0f,		0.0f } [0] );
	vertices.AddVertex(Vertex::AttributeFlag::position, &nm::float3{ 1.0f,		-1.0f,		0.0f } [0] );
	vertices.AddVertex(Vertex::AttributeFlag::position, &nm::float3{ 1.0f,		1.0f,		0.0f } [0] );
	vertices.AddVertex(Vertex::AttributeFlag::position, &nm::float3{ -1.0f,		1.0f,		0.0f } [0] );
	vertices.AddVertex(Vertex::AttributeFlag::position, &nm::float3{ -1.0f,		-1.0f,		1.0f } [0] );
	vertices.AddVertex(Vertex::AttributeFlag::position, &nm::float3{ 1.0f,		-1.0f,		1.0f } [0] );
	vertices.AddVertex(Vertex::AttributeFlag::position, &nm::float3{ 1.0f,		1.0f,		1.0f } [0] );
	vertices.AddVertex(Vertex::AttributeFlag::position, &nm::float3{ -1.0f,		1.0f,		1.0f } [0] );
	return vertices;
}

BFrustum::BFrustum(nm::float4x4 p_viewProj)
	: BVolume(BVolume::BType::Frustum)
	, viewProjection(p_viewProj)
{
}

/*
BFrustum BFrustum::operator*(nm::float4x4 const& p_transform)
{
	BFrustum frustum;
	for (int i = 0; i < 8; i++)
	{
		frustum.corners[i] = (p_transform * nm::float4(frustum.corners[i], 1.0f)).xyz();
	}
	return frustum;
}*/
