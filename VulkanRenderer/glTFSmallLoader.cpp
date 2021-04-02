#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE

#include "tiny_gltf.h"

#include "VulkanRenderer.h"
#include "Texture2D.h"
#include <glm/gtc/type_ptr.hpp>

/// <summary>
/// COURTESY OF Sascha THE GOD Willems
/// </summary>

class VulkanglTFModel
{
public:

	// NEEDED FOR COPY OF DATA
	VulkanRenderer* renderer;

	// THE VERTEX LAYOUT FOR THE SAMPLES' MODEL
	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;
		glm::vec3 color;
	};

	// VERTEX BUFFER FOR ALL PRIMITIVES
	struct {
		vk::Buffer buffer;
		vk::DeviceMemory memory;
	} vertices;

	// VERTEX BUFFER FOR ALL PRIMITIVES
	struct {
		int count;
		vk::Buffer buffer;
		vk::DeviceMemory memory;
	} indices;

	// REPRESENTATION OF GLTF SCENE STRUCTURE
	struct Node;

	// A PRIMITIVE CONTAINS THE DATA FOR A SINGLE DRAW CALL
	struct Primitive {
		uint32_t firstIndex;
		uint32_t indexCount;
		int32_t materialIndex;
	};

	// CONTAINS THE NODE'S (OPTIONAL) GEOMETRY AND CAN BE MADE UP OF AN ARBITRARY NUMBER OF PRIMITIVES
	struct Mesh {
		std::vector<Primitive> primitives;
	};

	// A NODE REPRESENTS AN OBJECT IN THE GLTF SCENE GRAPH
	// NODE == OBJECT
	struct Node {
		Node* parent;
		std::vector<Node> children;
		Mesh mesh;
		glm::mat4 matrix;
	};

	// A GLTF MATERIAL STORES INFORMATION IN E.G. THE TEXTURE THAT IS ATTACHED TO IT AND COLORS
	struct Material {
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		uint32_t baseColorTextureIndex;
	};

	// CONTAINS THE TEXTURE FOR A SINGLE GLTF IMAGE
	// IMAGES MAY BE REUSED BY TEXTURE OBJECTS AND ARE AS SUCH SEPARATED
	struct Image {
		Texture2D texture;
		// WE ALSO STORE (AND CREATE) A DESCRIPTOR SET THAT'S USED TO ACCESS THIS TEXTURE FROM THE FRAGMENT SHADER
		vk::DescriptorSet descriptorSet;
	};

	// A GLTF TEXTURE STORES A REFERENCE TO THE IMAGE AND A SAMPLER
	// IN THIS SAMPLE, WE ARE ONLY INTERESTED IN THE IMAGE
	struct Texture {
		int32_t imageIndex;
	};

	/*
		MODEL DATA
	*/
	std::vector<Image> images;
	std::vector<Texture> textures;
	std::vector<Material> materials;
	std::vector<Node> nodes;

	~VulkanglTFModel()
	{
		// RELEASE ALL VULKAN RESOURCES ALLOCATED FOR THE MODEL
		renderer->device->destroyBuffer(vertices.buffer);
		renderer->device->freeMemory(vertices.memory);
		renderer->device->destroyBuffer(indices.buffer);
		renderer->device->freeMemory(indices.memory);
		for (Image image : images) {
			renderer->device->destroyImageView(image.texture.view);
			renderer->device->destroyImage(image.texture.image);
			renderer->device->destroySampler(image.texture.sampler);
			renderer->device->freeMemory(image.texture.deviceMemory);
		}
	}

	void loadImages(tinygltf::Model& input)
	{
		// IMAGES CAN BE STORED INSIDE THE GLTF (WHICH IS THE CASE FOR THE SAMPLE MODEL), SO INSTEAD OF DIRECTLY
		// LOADING THEM FROM DISK, WE FETCH THEM FROM THE GLTF LOADER AND UPLOAD THE BUFFERS
		images.resize(input.images.size());
		for (size_t i = 0; i < input.images.size(); i++) {
			tinygltf::Image& glTFImage = input.images[i];
			// GET THE IMAGE DATA FROM THE GLTF LOADER
			unsigned char* buffer = nullptr;
			VkDeviceSize bufferSize = 0;
			bool deleteBuffer = false;
			// WE CONVERT RGB-ONLY IMAGES TO RGBA, AS MOST DEVICES DON'T SUPPORT RGB-FORMATS IN VULKAN
			if (glTFImage.component == 3) {
				bufferSize = glTFImage.width * glTFImage.height * 4;
				buffer = new unsigned char[bufferSize];
				unsigned char* rgba = buffer;
				unsigned char* rgb = &glTFImage.image[0];
				for (size_t i = 0; i < glTFImage.width * glTFImage.height; ++i) {
					memcpy(rgba, rgb, sizeof(unsigned char) * 3);
					rgba += 4;
					rgb += 3;
				}
				deleteBuffer = true;
			}
			else {
				buffer = &glTFImage.image[0];
				bufferSize = glTFImage.image.size();
			}
			// Load texture from image buffer
			// MAY NEED TO CHANGE TO R8G8B8A8_UNORM
			images[i].texture.loadFromBuffer(buffer, bufferSize , glTFImage.width, glTFImage.height, vk::Format::eR8G8B8A8Srgb);
			if (deleteBuffer) {
				delete buffer;
			}
		}
	}

	void loadTextures(tinygltf::Model& input)
	{
		textures.resize(input.textures.size());
		for (size_t i = 0; i < input.textures.size(); i++) {
			textures[i].imageIndex = input.textures[i].source;
		}
	}

	void loadMaterials(tinygltf::Model& input)
	{
		materials.resize(input.materials.size());
		for (size_t i = 0; i < input.materials.size(); i++) {
			// WE ONLY READ THE MOST BASIC PROPERTIES REQUIRED FOR OUR SAMPLE
			tinygltf::Material glTFMaterial = input.materials[i];
			// GET THE BASE COLOR FACTOR
			if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end()) {
				materials[i].baseColorFactor = glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
			}
			// GET BASE COLOR TEXTURE INDEX
			if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end()) {
				materials[i].baseColorTextureIndex = glTFMaterial.values["baseColorTexture"].TextureIndex();
			}
		}
	}

	void loadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, VulkanglTFModel::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<VulkanglTFModel::Vertex>& vertexBuffer)
	{
		VulkanglTFModel::Node node{};
		node.matrix = glm::mat4(1.0f);

		// GET THE LOCAL NODE MATRIX
		// IT'S EITHER MADE UP FROM TRANSLATION, ROTATION, SCALE OR A 4X4 MATRIX
		if (inputNode.translation.size() == 3) {
			node.matrix = glm::translate(node.matrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
		}
		if (inputNode.rotation.size() == 4) {
			glm::quat q = glm::make_quat(inputNode.rotation.data());
			node.matrix *= glm::mat4(q);
		}
		if (inputNode.scale.size() == 3) {
			node.matrix = glm::scale(node.matrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
		}
		if (inputNode.matrix.size() == 16) {
			node.matrix = glm::make_mat4x4(inputNode.matrix.data());
		};

		// LOAD NODE'S CHILDREN
		if (inputNode.children.size() > 0) {
			for (size_t i = 0; i < inputNode.children.size(); i++) {
				loadNode(input.nodes[inputNode.children[i]], input, &node, indexBuffer, vertexBuffer);
			}
		}

		// IF THE NODE CONTAINS MESH DATA, WE LOAD VERTICES AND INDICES FROM THE BUFFERS
		// IN GLTF THIS IS DONE VIA ACCESSORS AND BUFFER VIEWS
		if (inputNode.mesh > -1) {
			const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];
			// ITERATE THROUGH ALL PRIMITIVES OF THIS NODE'S MESH
			for (size_t i = 0; i < mesh.primitives.size(); i++) {
				const tinygltf::Primitive& glTFPrimitive = mesh.primitives[i];
				uint32_t firstIndex = static_cast<uint32_t>(indexBuffer.size());
				uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
				uint32_t indexCount = 0;
				// VERTICES
				{
					const float* positionBuffer = nullptr;
					const float* normalsBuffer = nullptr;
					const float* texCoordsBuffer = nullptr;
					size_t vertexCount = 0;

					// GET BUFFER DATA FOR VERTEX NORMALS
					if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end()) {
						const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
						const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
						positionBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
						vertexCount = accessor.count;
					}
					// GET BUFFER DATA FOR VERTEX NORMALS
					if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end()) {
						const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
						const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
						normalsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					}
					// GET BUFFER DATA FOR VERTEX TEXTURE COORDINATES
					// GLTF SUPPORTS MULTIPLE SETS, WE ONLY LOAD THE FIRST ONE
					if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end()) {
						const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
						const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
						texCoordsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					}

					// APPEND DATA TO MODEL'S VERTEX BUFFER
					for (size_t v = 0; v < vertexCount; v++) {
						Vertex vert{};
						vert.pos = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
						vert.normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
						vert.uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
						vert.color = glm::vec3(1.0f);
						vertexBuffer.push_back(vert);
					}
				}
				// INDICES
				{
					const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.indices];
					const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];

					indexCount += static_cast<uint32_t>(accessor.count);

					// GLTF SUPPORTS DIFFERENT COMPONENT TYPES OF INDICES
					switch (accessor.componentType) {
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
						uint32_t* buf = new uint32_t[accessor.count];
						memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint32_t));
						for (size_t index = 0; index < accessor.count; index++) {
							indexBuffer.push_back(buf[index] + vertexStart);
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
						uint16_t* buf = new uint16_t[accessor.count];
						memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint16_t));
						for (size_t index = 0; index < accessor.count; index++) {
							indexBuffer.push_back(buf[index] + vertexStart);
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
						uint8_t* buf = new uint8_t[accessor.count];
						memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint8_t));
						for (size_t index = 0; index < accessor.count; index++) {
							indexBuffer.push_back(buf[index] + vertexStart);
						}
						break;
					}
					default:
						std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
						return;
					}
				}
				Primitive primitive{};
				primitive.firstIndex = firstIndex;
				primitive.indexCount = indexCount;
				primitive.materialIndex = glTFPrimitive.material;
				node.mesh.primitives.push_back(primitive);
			}
		}

		if (parent) {
			parent->children.push_back(node);
		}
		else {
			nodes.push_back(node);
		}
	}

	/*
		GLTF RENDERING FUNCTIONS
	*/

	// DRAW A SINGLE NODE INCLUDING CHILD NODES (IF PRESENT)
	void drawNode(VulkanRenderer* targetRenderer, int cmdBuffIndex,VulkanglTFModel::Node node)
	{
		if (node.mesh.primitives.size() > 0) {
			// PASS THE NODE'S MATRIX VIA PUSH CONSTANTS
			// TRAVERSE THE NODE HIERARCHY TO THE TOP-MOST PARENT TO GET THE FINAL MATRIX OF THE CURRENT NODE
			glm::mat4 nodeMatrix = node.matrix;
			VulkanglTFModel::Node* currentParent = node.parent;
			while (currentParent) {
				nodeMatrix = currentParent->matrix * nodeMatrix;
				currentParent = currentParent->parent;
			}
			// PASS THE FINAL MATRIX TO THE VERTEX SHADER USING PUSH CONSTANTS
			targetRenderer->commandBuffers[cmdBuffIndex].pushConstants(targetRenderer->pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), &nodeMatrix);
			for (VulkanglTFModel::Primitive& primitive : node.mesh.primitives) {
				if (primitive.indexCount > 0) {
					// GET THE TEXTURE INDEX FOR THIS PRIMITIVE
					VulkanglTFModel::Texture texture = textures[materials[primitive.materialIndex].baseColorTextureIndex];
					// BIND THE DESCRIPTOR FOR THE CURRENT PRIMITIVE'S TEXTURE
					targetRenderer->commandBuffers[cmdBuffIndex].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, targetRenderer->pipelineLayout, 1, 1, &images[texture.imageIndex].descriptorSet, 0, nullptr);
					targetRenderer->commandBuffers[cmdBuffIndex].drawIndexed(primitive.indexCount, 1, primitive.firstIndex, 0, 0);
				}
			}
		}
		for (auto& child : node.children) {
			drawNode(targetRenderer, cmdBuffIndex,  child);
		}
	}

};