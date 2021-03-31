#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE

#include "tiny_gltf.h"

#include "VulkanRenderer.h"

/// <summary>
/// COURTESY OF Sascha THE GOD Willems
/// </summary>

class VulkanglTFModel
{
public:

	// NEEDED FOR COPY OF DATA
	vk::Device* vulkanDevice;
	vk::Queue copyQueue;

	// The vertex layout for the samples' model
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

	// Contains the texture for a single glTF image
	// Images may be reused by texture objects and are as such separated
	struct Image {
		// vk::Texture2D texture;
		// We also store (and create) a descriptor set that's used to access this texture from the fragment shader
		VkDescriptorSet descriptorSet;
	};

	// A glTF texture stores a reference to the image and a sampler
	// In this sample, we are only interested in the image
	struct Texture {
		int32_t imageIndex;
	};

	/*
		Model data
	*/
	std::vector<Image> images;
	std::vector<Texture> textures;
	std::vector<Material> materials;
	std::vector<Node> nodes;



};