#include "stdafx.h"
#include <cstdlib>
#include <stdio.h>
#include <fstream>
#include <iostream>

#include "Scene.h"
#include "GameException.h"
#include "Utility.h"
#include "Model.h"
#include "Materials.inl"
#include "ER_RenderingObject.h"
#include "MaterialHelper.h"
#include "ER_Illumination.h"
#include "ER_LightProbesManager.h"
#include "ER_FoliageManager.h"
#include "DirectionalLight.h"

#define MULTITHREADED_SCENE_LOAD 1

namespace Library 
{
	Scene::Scene(Game& pGame, Camera& pCamera, const std::string& path) :
		GameComponent(pGame), mCamera(pCamera), mScenePath(path)
	{
		Json::Reader reader;
		std::ifstream scene(path.c_str(), std::ifstream::binary);

		if (!reader.parse(scene, root)) {
			throw GameException(reader.getFormattedErrorMessages().c_str());
		}
		else {

			if (root.isMember("camera_position")) {
				float vec3[3];
				for (Json::Value::ArrayIndex i = 0; i != root["camera_position"].size(); i++)
					vec3[i] = root["camera_position"][i].asFloat();

				cameraPosition = XMFLOAT3(vec3[0], vec3[1], vec3[2]);
			}

			if (root.isMember("camera_direction")) {
				float vec3[3];
				for (Json::Value::ArrayIndex i = 0; i != root["camera_direction"].size(); i++)
					vec3[i] = root["camera_direction"][i].asFloat();

				cameraDirection = XMFLOAT3(vec3[0], vec3[1], vec3[2]);
			}

			if (root.isMember("sun_direction")) {
				float vec3[3];
				for (Json::Value::ArrayIndex i = 0; i != root["sun_direction"].size(); i++)
					vec3[i] = root["sun_direction"][i].asFloat();

				sunDirection = XMFLOAT3(vec3[0], vec3[1], vec3[2]);
			}

			if (root.isMember("sun_color")) {
				float vec3[3];
				for (Json::Value::ArrayIndex i = 0; i != root["sun_color"].size(); i++)
					vec3[i] = root["sun_color"][i].asFloat();

				sunColor = XMFLOAT3(vec3[0], vec3[1], vec3[2]);
			}

			if (root.isMember("ambient_color")) {
				float vec3[3];
				for (Json::Value::ArrayIndex i = 0; i != root["ambient_color"].size(); i++)
					vec3[i] = root["ambient_color"][i].asFloat();

				ambientColor = XMFLOAT3(vec3[0], vec3[1], vec3[2]);
			}

			if (root.isMember("skybox_path")) {
				skyboxPath = root["skybox_path"].asString();
			}

			// light probes config
			{
				if (root.isMember("light_probes_volume_bounds_min")) {
					float vec3[3];
					for (Json::Value::ArrayIndex i = 0; i != root["light_probes_volume_bounds_min"].size(); i++)
						vec3[i] = root["light_probes_volume_bounds_min"][i].asFloat();

					mLightProbesVolumeMinBounds = XMFLOAT3(vec3[0], vec3[1], vec3[2]);
				}
				else
					mHasLightProbes = false;

				if (root.isMember("light_probes_volume_bounds_max")) {
					float vec3[3];
					for (Json::Value::ArrayIndex i = 0; i != root["light_probes_volume_bounds_max"].size(); i++)
						vec3[i] = root["light_probes_volume_bounds_max"][i].asFloat();

					mLightProbesVolumeMaxBounds = XMFLOAT3(vec3[0], vec3[1], vec3[2]);
				}
				else
					mHasLightProbes = false;

				if (root.isMember("light_probes_diffuse_distance"))
					mLightProbesDiffuseDistance = root["light_probes_diffuse_distance"].asFloat();
				if (root.isMember("light_probes_specular_distance"))
					mLightProbesSpecularDistance = root["light_probes_specular_distance"].asFloat();
			}

			if (root.isMember("foliage_zones"))
				mHasFoliage = true;

			if (root.isMember("use_volumetric_fog")) {
				mHasVolumetricFog = root["use_volumetric_fog"].asBool();
			}
			else
				mHasVolumetricFog = false;

			// add rendering objects to scene
			unsigned int numRenderingObjects = root["rendering_objects"].size();
			for (Json::Value::ArrayIndex i = 0; i != numRenderingObjects; i++) {
				std::string name = root["rendering_objects"][i]["name"].asString();
				std::string modelPath = root["rendering_objects"][i]["model_path"].asString();
				bool isInstanced = root["rendering_objects"][i]["instanced"].asBool();
				objects.emplace(name, new ER_RenderingObject(name, i, *mGame, mCamera, std::unique_ptr<Model>(new Model(*mGame, Utility::GetFilePath(modelPath), true)), true, isInstanced));
			}

			assert(numRenderingObjects == objects.size());

#if MULTITHREADED_SCENE_LOAD
			int numThreads = std::thread::hardware_concurrency();
#else
			int numThreads = 1;
#endif
			int objectsPerThread = numRenderingObjects / numThreads;
			if (objectsPerThread == 0)
			{
				numThreads = 1;
				objectsPerThread = numRenderingObjects;
			}

			std::vector<std::thread> threads;
			threads.reserve(numThreads);
			
			for (int i = 0; i < numThreads; i++)
			{
				threads.push_back(std::thread([&, numThreads, numRenderingObjects, objectsPerThread, i]
				{
					int endRange = (i < numThreads - 1) ? (i + 1) * objectsPerThread : numRenderingObjects;
					
					for (int j = i * objectsPerThread; j < endRange; j++)
					{
						auto objectI = objects.begin();
						std::advance(objectI, j);
						LoadRenderingObjectData(objectI->second);
					}
				}));
			}
			for (auto& t : threads) t.join();
			
			for (auto& obj : objects)
				LoadRenderingObjectInstancedData(obj.second);
		}
	}

	Scene::~Scene()
	{
		for (auto object : objects)
		{
			object.second->MeshMaterialVariablesUpdateEvent->RemoveAllListeners();
			DeleteObject(object.second);
		}
		objects.clear();
	}

	void Scene::LoadRenderingObjectData(ER_RenderingObject* aObject)
	{
		if (!aObject)
			return;

		int i = aObject->GetIndexInScene();
		bool isInstanced = aObject->IsInstanced();
		bool hasLODs = false;

		// load flags
		{
			if (root["rendering_objects"][i].isMember("foliageMask"))
				aObject->SetFoliageMask(root["rendering_objects"][i]["foliageMask"].asBool());
			if (root["rendering_objects"][i].isMember("use_indirect_global_lightprobe"))
				aObject->SetUseIndirectGlobalLightProbe(root["rendering_objects"][i]["use_indirect_global_lightprobe"].asBool());
			if (root["rendering_objects"][i].isMember("use_in_global_lightprobe_rendering"))
				aObject->SetIsUsedForGlobalLightProbeRendering(root["rendering_objects"][i]["use_in_global_lightprobe_rendering"].asBool());
			if (root["rendering_objects"][i].isMember("use_parallax_occlusion_mapping"))
				aObject->SetParallaxOcclusionMapping(root["rendering_objects"][i]["use_parallax_occlusion_mapping"].asBool());
			if (root["rendering_objects"][i].isMember("use_forward_shading"))
				aObject->SetForwardShading(root["rendering_objects"][i]["use_forward_shading"].asBool());
			if (root["rendering_objects"][i].isMember("placed_on_terrain")) {
				aObject->SetPlacedOnTerrain(root["rendering_objects"][i]["placed_on_terrain"].asBool());
				if (isInstanced && root["rendering_objects"][i].isMember("num_instances_per_vegetation_zone"))
					aObject->SetNumInstancesPerVegetationZone(root["rendering_objects"][i]["num_instances_per_vegetation_zone"].asInt());
			}
			if (root["rendering_objects"][i].isMember("min_scale"))
				aObject->SetMinScale(root["rendering_objects"][i]["min_scale"].asFloat());
			if (root["rendering_objects"][i].isMember("max_scale"))
				aObject->SetMaxScale(root["rendering_objects"][i]["max_scale"].asFloat());
		}

		// load materials
		{
			if (root["rendering_objects"][i].isMember("new_materials")) {
				unsigned int numMaterials = root["rendering_objects"][i]["new_materials"].size();
				for (Json::Value::ArrayIndex matIndex = 0; matIndex != numMaterials; matIndex++) {
					std::string name = root["rendering_objects"][i]["new_materials"][matIndex]["name"].asString();

					MaterialShaderEntries shaderEntries;
					if (root["rendering_objects"][i]["new_materials"][matIndex].isMember("vertexEntry"))
						shaderEntries.vertexEntry = root["rendering_objects"][i]["new_materials"][matIndex]["vertexEntry"].asString();
					if (root["rendering_objects"][i]["new_materials"][matIndex].isMember("geometryEntry"))
						shaderEntries.geometryEntry = root["rendering_objects"][i]["new_materials"][matIndex]["geometryEntry"].asString();
					if (root["rendering_objects"][i]["new_materials"][matIndex].isMember("hullEntry"))
						shaderEntries.hullEntry = root["rendering_objects"][i]["new_materials"][matIndex]["hullEntry"].asString();	
					if (root["rendering_objects"][i]["new_materials"][matIndex].isMember("domainEntry"))
						shaderEntries.domainEntry = root["rendering_objects"][i]["new_materials"][matIndex]["domainEntry"].asString();	
					if (root["rendering_objects"][i]["new_materials"][matIndex].isMember("pixelEntry"))
						shaderEntries.pixelEntry = root["rendering_objects"][i]["new_materials"][matIndex]["pixelEntry"].asString();

					if (isInstanced) //be careful with the instancing support in shaders of the materials! (i.e., maybe the material does not have instancing entry point/support)
						shaderEntries.vertexEntry = shaderEntries.vertexEntry + "_instancing";
					
					if (name == MaterialHelper::gbufferMaterialName)
						aObject->SetInGBuffer(true);
					if (name == MaterialHelper::renderToLightProbeMaterialName)
						aObject->SetInLightProbe(true);

					if (name == MaterialHelper::shadowMapMaterialName)
					{
						for (int cascade = 0; cascade < NUM_SHADOW_CASCADES; cascade++)
						{
							std::string cascadedname = MaterialHelper::shadowMapMaterialName + " " + std::to_string(cascade);
							aObject->LoadMaterial(GetMaterialByName(name, shaderEntries, isInstanced), cascadedname);
						}
					}
					else if (name == MaterialHelper::voxelizationMaterialName)
					{
						aObject->SetInVoxelization(true);
						for (int cascade = 0; cascade < NUM_VOXEL_GI_CASCADES; cascade++)
						{
							const std::string fullname = MaterialHelper::voxelizationMaterialName + "_" + std::to_string(cascade);
							aObject->LoadMaterial(GetMaterialByName(name, shaderEntries, isInstanced), fullname);
						}
					}
					else if (name == MaterialHelper::renderToLightProbeMaterialName)
					{
						std::string originalPSEntry = shaderEntries.pixelEntry;
						for (int cubemapFaceIndex = 0; cubemapFaceIndex < CUBEMAP_FACES_COUNT; cubemapFaceIndex++)
						{
							std::string newName;
							//diffuse
							{
								shaderEntries.pixelEntry = originalPSEntry + "_DiffuseProbes";
								newName = "diffuse_" + MaterialHelper::renderToLightProbeMaterialName + "_" + std::to_string(cubemapFaceIndex);
								aObject->LoadMaterial(GetMaterialByName(name, shaderEntries, isInstanced), newName);
							}
							//specular
							{
								shaderEntries.pixelEntry = originalPSEntry + "_SpecularProbes";
								newName = "specular_" + MaterialHelper::renderToLightProbeMaterialName + "_" + std::to_string(cubemapFaceIndex);
								aObject->LoadMaterial(GetMaterialByName(name, shaderEntries, isInstanced), newName);
							}
						}
					}
					else //other standard materials
						aObject->LoadMaterial(GetMaterialByName(name, shaderEntries, isInstanced), name);
				}
			}

			aObject->LoadRenderBuffers();
		}

		// load custom textures
		{
			if (root["rendering_objects"][i].isMember("textures")) {

				for (Json::Value::ArrayIndex mesh = 0; mesh != root["rendering_objects"][i]["textures"].size(); mesh++) {
					if (root["rendering_objects"][i]["textures"][mesh].isMember("albedo"))
						aObject->mCustomAlbedoTextures[mesh] = root["rendering_objects"][i]["textures"][mesh]["albedo"].asString();
					if (root["rendering_objects"][i]["textures"][mesh].isMember("normal"))
						aObject->mCustomNormalTextures[mesh] = root["rendering_objects"][i]["textures"][mesh]["normal"].asString();
					if (root["rendering_objects"][i]["textures"][mesh].isMember("roughness"))
						aObject->mCustomRoughnessTextures[mesh] = root["rendering_objects"][i]["textures"][mesh]["roughness"].asString();
					if (root["rendering_objects"][i]["textures"][mesh].isMember("metalness"))
						aObject->mCustomMetalnessTextures[mesh] = root["rendering_objects"][i]["textures"][mesh]["metalness"].asString();
					if (root["rendering_objects"][i]["textures"][mesh].isMember("height"))
						aObject->mCustomHeightTextures[mesh] = root["rendering_objects"][i]["textures"][mesh]["height"].asString();
					if (root["rendering_objects"][i]["textures"][mesh].isMember("reflection_mask"))
						aObject->mCustomReflectionMaskTextures[mesh] = root["rendering_objects"][i]["textures"][mesh]["reflection_mask"].asString();

					aObject->LoadCustomMeshTextures(mesh);
				}
			}
		}

		// load world transform
		{
			if (root["rendering_objects"][i].isMember("transform")) {
				if (root["rendering_objects"][i]["transform"].size() != 16)
				{
					aObject->SetTransformationMatrix(XMMatrixIdentity());
				}
				else {
					float matrix[16];
					for (Json::Value::ArrayIndex matC = 0; matC != root["rendering_objects"][i]["transform"].size(); matC++) {
						matrix[matC] = root["rendering_objects"][i]["transform"][matC].asFloat();
					}
					XMFLOAT4X4 worldTransform(matrix);
					aObject->SetTransformationMatrix(XMMatrixTranspose(XMLoadFloat4x4(&worldTransform)));
				}
			}
			else
				aObject->SetTransformationMatrix(XMMatrixIdentity());
		}

		// load lods
		{
			hasLODs = root["rendering_objects"][i].isMember("model_lods");
			if (hasLODs) {
				for (Json::Value::ArrayIndex lod = 1 /* 0 is main model loaded before */; lod != root["rendering_objects"][i]["model_lods"].size(); lod++) {
					std::string path = root["rendering_objects"][i]["model_lods"][lod]["path"].asString();
					aObject->LoadLOD(std::unique_ptr<Model>(new Model(*mGame, Utility::GetFilePath(path), true)));
				}
			}
		}
	}

	// [WARNING] NOT THREAD-SAFE!
	void Scene::LoadRenderingObjectInstancedData(ER_RenderingObject* aObject)
	{
		int i = aObject->GetIndexInScene();
		bool isInstanced = aObject->IsInstanced();
		bool hasLODs = root["rendering_objects"][i].isMember("model_lods");
		if (isInstanced) {
			if (hasLODs)
			{
				for (int lod = 0; lod < root["rendering_objects"][i]["model_lods"].size(); lod++)
				{
					aObject->LoadInstanceBuffers(lod);
					if (root["rendering_objects"][i].isMember("instances_transforms")) {
						aObject->ResetInstanceData(root["rendering_objects"][i]["instances_transforms"].size(), true, lod);
						for (Json::Value::ArrayIndex instance = 0; instance != root["rendering_objects"][i]["instances_transforms"].size(); instance++) {
							float matrix[16];
							for (Json::Value::ArrayIndex matC = 0; matC != root["rendering_objects"][i]["instances_transforms"][instance]["transform"].size(); matC++) {
								matrix[matC] = root["rendering_objects"][i]["instances_transforms"][instance]["transform"][matC].asFloat();
							}
							XMFLOAT4X4 worldTransform(matrix);
							aObject->AddInstanceData(XMMatrixTranspose(XMLoadFloat4x4(&worldTransform)), lod);
						}
					}
					else {
						aObject->ResetInstanceData(1, true, lod);
						aObject->AddInstanceData(aObject->GetTransformationMatrix(), lod);
					}
					aObject->UpdateInstanceBuffer(aObject->GetInstancesData(), lod);
				}
			}
			else {
				aObject->LoadInstanceBuffers();
				if (root["rendering_objects"][i].isMember("instances_transforms")) {
					aObject->ResetInstanceData(root["rendering_objects"][i]["instances_transforms"].size(), true);
					for (Json::Value::ArrayIndex instance = 0; instance != root["rendering_objects"][i]["instances_transforms"].size(); instance++) {
						float matrix[16];
						for (Json::Value::ArrayIndex matC = 0; matC != root["rendering_objects"][i]["instances_transforms"][instance]["transform"].size(); matC++) {
							matrix[matC] = root["rendering_objects"][i]["instances_transforms"][instance]["transform"][matC].asFloat();
						}
						XMFLOAT4X4 worldTransform(matrix);
						aObject->AddInstanceData(XMMatrixTranspose(XMLoadFloat4x4(&worldTransform)));
					}
				}
				else {
					aObject->ResetInstanceData(1, true);
					aObject->AddInstanceData(aObject->GetTransformationMatrix());
				}
				aObject->UpdateInstanceBuffer(aObject->GetInstancesData());
			}
		}
	}

	void Scene::SaveFoliageZonesTransforms(const std::vector<ER_Foliage*>& foliageZones)
	{
		if (mScenePath.empty())
			throw GameException("Can't save to scene json file! Empty scene name...");

		if (root.isMember("foliage_zones")) {
			assert(foliageZones.size() == root["foliage_zones"].size());
			float vec3[3];
			for (Json::Value::ArrayIndex iz = 0; iz != root["foliage_zones"].size(); iz++)
			{
				Json::Value content(Json::arrayValue);
				vec3[0] = foliageZones[iz]->GetDistributionCenter().x;
				vec3[1] = foliageZones[iz]->GetDistributionCenter().y;
				vec3[2] = foliageZones[iz]->GetDistributionCenter().z;
				for (Json::Value::ArrayIndex i = 0; i < 3; i++)
					content.append(vec3[i]);
				root["foliage_zones"][iz]["position"] = content;
			}
		}

		Json::StreamWriterBuilder builder;
		std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

		std::ofstream file_id;
		file_id.open(mScenePath.c_str());
		writer->write(root, &file_id);
	}

	void Scene::SaveRenderingObjectsTransforms()
	{
		if (mScenePath.empty())
			throw GameException("Can't save to scene json file! Empty scene name...");

		// store world transform
		for (Json::Value::ArrayIndex i = 0; i != root["rendering_objects"].size(); i++) {
			Json::Value content(Json::arrayValue);
			if (root["rendering_objects"][i].isMember("transform")) {
				XMFLOAT4X4 mat = objects[root["rendering_objects"][i]["name"].asString()]->GetTransformationMatrix4X4();
				XMMATRIX matXM  = XMMatrixTranspose(XMLoadFloat4x4(&mat));
				XMStoreFloat4x4(&mat, matXM);
				float matF[16];
				MatrixHelper::GetFloatArray(mat, matF);
				for (int i = 0; i < 16; i++)
					content.append(matF[i]);
				root["rendering_objects"][i]["transform"] = content;
			}
			else
			{
				float matF[16];
				XMFLOAT4X4 mat;
				XMStoreFloat4x4(&mat, XMMatrixTranspose(XMMatrixIdentity()));
				MatrixHelper::GetFloatArray(mat, matF);

				for (int i = 0; i < 16; i++)
					content.append(matF[i]);
				root["rendering_objects"][i]["transform"] = content;
			}

			if (root["rendering_objects"][i].isMember("instanced")) 
			{
				bool isInstanced = root["rendering_objects"][i]["instanced"].asBool();
				if (isInstanced)
				{
					if (root["rendering_objects"][i].isMember("instances_transforms")) {

						if (root["rendering_objects"][i]["instances_transforms"].size() != objects[root["rendering_objects"][i]["name"].asString()]->GetInstanceCount())
							throw GameException("Can't save instances transforms to scene json file! RenderObject's instance count is not equal to the number of instance transforms in scene file.");

						for (Json::Value::ArrayIndex instance = 0; instance != root["rendering_objects"][i]["instances_transforms"].size(); instance++) {
							Json::Value contentInstanceTransform(Json::arrayValue);

							XMFLOAT4X4 mat = objects[root["rendering_objects"][i]["name"].asString()]->GetInstancesData()[instance].World;
							XMMATRIX matXM = XMMatrixTranspose(XMLoadFloat4x4(&mat));
							XMStoreFloat4x4(&mat, matXM);
							float matF[16];
							MatrixHelper::GetFloatArray(mat, matF);
							for (int i = 0; i < 16; i++)
								contentInstanceTransform.append(matF[i]);

							root["rendering_objects"][i]["instances_transforms"][instance]["transform"] = contentInstanceTransform;
						}
					}
				}
			}
		}

		Json::StreamWriterBuilder builder;
		std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

		std::ofstream file_id;
		file_id.open(mScenePath.c_str());
		writer->write(root, &file_id);
	}

	ER_Material* Scene::GetMaterialByName(const std::string& matName, const MaterialShaderEntries& entries, bool instanced)
	{
		auto core = GetGame();
		assert(core);

		ER_Material* material = nullptr;

		// cant do reflection in C++
		if (matName == "BasicColorMaterial")
			material = new ER_BasicColorMaterial(*core, entries, HAS_VERTEX_SHADER | HAS_PIXEL_SHADER /*TODO instanced support*/);
		else if (matName == "ShadowMapMaterial")
			material = new ER_ShadowMapMaterial(*core, entries, HAS_VERTEX_SHADER | HAS_PIXEL_SHADER, instanced);
		else if (matName == "GBufferMaterial")
			material = new ER_GBufferMaterial(*core, entries, HAS_VERTEX_SHADER | HAS_PIXEL_SHADER, instanced);
		else if (matName == "RenderToLightProbeMaterial")
			material = new ER_RenderToLightProbeMaterial(*core, entries, HAS_VERTEX_SHADER | HAS_PIXEL_SHADER, instanced);
		else if (matName == "VoxelizationMaterial")
			material = new ER_VoxelizationMaterial(*core, entries, HAS_VERTEX_SHADER | HAS_GEOMETRY_SHADER | HAS_PIXEL_SHADER, instanced);
		else
			material = nullptr;

		return material;
	}

	void Scene::LoadFoliageZones(std::vector<ER_Foliage*>& foliageZones, DirectionalLight& light)
	{
		Json::Reader reader;
		std::ifstream scene(mScenePath.c_str(), std::ifstream::binary);
		Game* core = GetGame();
		assert(core);

		if (!reader.parse(scene, root)) {
			throw GameException(reader.getFormattedErrorMessages().c_str());
		}
		else {
			if (root.isMember("foliage_zones")) {
				for (Json::Value::ArrayIndex i = 0; i != root["foliage_zones"].size(); i++)
				{
					float vec3[3];
					for (Json::Value::ArrayIndex ia = 0; ia != root["foliage_zones"][i]["position"].size(); ia++)
						vec3[ia] = root["foliage_zones"][i]["position"][ia].asFloat();

					foliageZones.push_back(new ER_Foliage(*core, mCamera, light,
						root["foliage_zones"][i]["patch_count"].asInt(),
						Utility::GetFilePath(root["foliage_zones"][i]["texture_path"].asString()),
						root["foliage_zones"][i]["average_scale"].asFloat(),
						root["foliage_zones"][i]["distribution_radius"].asFloat(),
						XMFLOAT3(vec3[0], vec3[1], vec3[2]),
						(FoliageBillboardType)root["foliage_zones"][i]["type"].asInt()));
				}
			}
			else
				mHasFoliage = false;
		}
	}

}