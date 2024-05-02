#include "TPch.h"

#include "ScriptingEngine.h"
#include "ScriptGlue.h"
#include <fstream>
#include "spdlog/fmt/ostr.h"

#include "Tso/Scene/Component.h"


#include "mono/jit/jit.h"
#include "mono/metadata/assembly.h"
#include "mono/metadata/object.h"
#include "mono/metadata/tabledefs.h"
#include "mono/metadata/mono-debug.h"
#include "mono/metadata/threads.h"
#include "mono/metadata/image.h"

namespace Tso {
static std::unordered_map<std::string, ScriptFieldType> s_ScriptFieldTypeMap =
{
	{ "System.Single", ScriptFieldType::Float },
	{ "System.Double", ScriptFieldType::Double },
	{ "System.Boolean", ScriptFieldType::Bool },
	{ "System.Char", ScriptFieldType::Char },
	{ "System.Int16", ScriptFieldType::Short },
	{ "System.Int32", ScriptFieldType::Int },
	{ "System.Int64", ScriptFieldType::Long },
	{ "System.Byte", ScriptFieldType::Byte },
	{ "System.UInt16", ScriptFieldType::UShort },
	{ "System.UInt32", ScriptFieldType::UInt },
	{ "System.UInt64", ScriptFieldType::ULong },

	{ "Tso.Vector2", ScriptFieldType::Vector2 },
	{ "Tso.Vector3", ScriptFieldType::Vector3 },
	{ "Tso.Vector4", ScriptFieldType::Vector4 },

	{ "Tso.Entity", ScriptFieldType::Entity },
	{ "Tso.SpriteAnimationIdle", ScriptFieldType::SpriteAnimationIdle },

};

namespace Utils {
	static char* ReadBytes(const std::string& filepath, uint32_t* outSize)
	{
		std::ifstream stream(filepath, std::ios::binary | std::ios::ate);

		if (!stream)
		{
			TSO_CORE_ERROR("Failed to open the file:" + filepath);
			return nullptr;
		}

		std::streampos end = stream.tellg();
		stream.seekg(0, std::ios::beg);
		uint32_t size = (uint32_t)(end - stream.tellg());

		if (!size)
		{
			TSO_CORE_ERROR("File is empty:" + filepath);
			return nullptr;
		}

		char* buffer = new char[size];
		stream.read((char*)buffer, size);
		stream.close();

		*outSize = size;
		return buffer;
	}



	static MonoAssembly* LoadMonoAssembly(const std::filesystem::path& assemblyPath, bool loadPDB = false)
	{
		uint32_t fileSize = 0;
		char* fileData = ReadBytes(assemblyPath.string(), &fileSize);

		// NOTE: We can't use this image for anything other than loading the assembly because this image doesn't have a reference to the assembly
		MonoImageOpenStatus status;
		MonoImage* image = mono_image_open_from_data_full(fileData, fileSize, 1, &status, 0);

		if (status != MONO_IMAGE_OK)
		{
			const char* errorMessage = mono_image_strerror(status);
			// Log some error message using the errorMessage data
			return nullptr;
		}

		/*if (loadPDB)
		{
			std::filesystem::path pdbPath = assemblyPath;
			pdbPath.replace_extension(".pdb");

			if (std::filesystem::exists(pdbPath))
			{
				uint32_t fileSize = 0;
				char* pdbFileData = ReadBytes(pdbPath.string() , &fileSize);
				mono_debug_open_image_from_memory(image, (mono_byte*)pdbFileData, fileSize);
				TSO_CORE_INFO("Loaded PDB {}", pdbPath);
			}
		}*/

		std::string pathString = assemblyPath.string();
		MonoAssembly* assembly = mono_assembly_load_from_full(image, pathString.c_str(), &status, 0);
		mono_image_close(image);

		return assembly;
	}

	void PrintAssemblyTypes(MonoAssembly* assembly)
	{
		MonoImage* image = mono_assembly_get_image(assembly);
		const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
		int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

		for (int32_t i = 0; i < numTypes; i++)
		{
			uint32_t cols[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

			const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
			const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);
			TSO_CORE_TRACE("{}.{}", nameSpace, name);
		}
	}

	ScriptFieldType MonoTypeToScriptFieldType(MonoType* monoType)
	{
		std::string typeName = mono_type_get_name(monoType);

		auto it = s_ScriptFieldTypeMap.find(typeName);
		if (it == s_ScriptFieldTypeMap.end())
		{
			TSO_CORE_ERROR("Unknown type: {}", typeName);
			return ScriptFieldType::None;
		}

		return it->second;
	}
}



	struct ScriptEngineData {
		MonoDomain* RootDomain = nullptr;
		MonoDomain* AppDomain = nullptr;


		MonoAssembly* CoreAssembly = nullptr;
		MonoImage* CoreAssemblyImage = nullptr;
		std::filesystem::path CoreAssemblyFilepath;

		MonoAssembly* AppAssembly = nullptr;
		MonoImage* AppAssemblyImage = nullptr;
		std::filesystem::path AppAssemblyFilepath;

		ScriptClass EntityClass;



		std::unordered_map<std::string, Ref<ScriptClass>> EntityClasses;
		std::unordered_map<uint64_t, Ref<ScriptInstance>> EntityInstances;

		std::unordered_map<uint64_t, ScriptFieldMap> EntityScriptFields;

		Scene* SceneContext = nullptr;
	};

	static ScriptEngineData* s_Data = nullptr;
	 

	void ScriptingEngine::Init()
	{
		s_Data = new ScriptEngineData;
		InitMono();
		ScriptGlue::RegisterFunctions();
	}



	void ScriptingEngine::InitMono()
	{
		mono_set_assemblies_path("../TsoEngine/third_party/mono/DotNetLibs/4.5");
		//TODO: consider add filesystem to make file accessible@SuchanTso

		MonoDomain* rootDomain = mono_jit_init("TsoJitRuntime");
		TSO_CORE_ASSERT(rootDomain, "failed to init root Domain");
		s_Data->RootDomain = rootDomain;
		//mono_thread_set_main(mono_thread_current());
		bool status = LoadAssembly(std::filesystem::path("../TsoEngine-ScriptCore/Build/TsoEngine-ScriptCore.dll"));
		if (!status) {
			TSO_CORE_ERROR("unable to load assembly");
		}

		LoadAssemblyClasses();

		s_Data->EntityClass = ScriptClass("Tso", "Entity", true);
		/*ScriptEngineData* testptr = s_Data;
		int a = 0;*/
#if 0
		if (LoadAssembly(std::filesystem::path("../TsoEngine-ScriptCore/Build/TsoEngine-ScriptCore.dll"))) {
			//MonoImage* image = mono_assembly_get_image(s_Data->CoreAssembly);
			MonoClass* klass = mono_class_from_name(s_Data->CoreAssemblyImage, "MyNamespace", "Program");
			if (klass) {
				MonoObject* objecteInstance = mono_object_new(s_Data->AppDomain,klass);
				MonoMethod* objectMethod = mono_class_get_method_from_name(klass, "PrintFloatVar", 0);
				if (objectMethod) {
					mono_runtime_invoke(objectMethod, objecteInstance, nullptr, nullptr);
				}
				MonoMethod* objectMothedP = mono_class_get_method_from_name(klass, "PrintFloatP", 1);
				float value = 3.14159f;
				void* params[] = {&value};
				mono_runtime_invoke(objectMothedP, objecteInstance, params, nullptr);
			}
		}
#endif
	}

	void ScriptingEngine::ShutDown()
	{
		ShutdownMono();
		delete s_Data;
	}

	void ScriptingEngine::ShutdownMono()
	{
		mono_domain_set(mono_get_root_domain(), false);

		mono_domain_unload(s_Data->AppDomain);
		s_Data->AppDomain = nullptr;

		mono_jit_cleanup(s_Data->RootDomain);
		s_Data->RootDomain = nullptr;
	}

	void ScriptingEngine::OnCreateEntity(Entity entity)
	{
		const auto& sc = entity.GetComponent<ScriptComponent>();
		if (ScriptingEngine::EntityClassExists(sc.ClassName))
		{
			auto entityID = entity.GetUUID();

			Ref<ScriptInstance> instance = std::make_shared<ScriptInstance>(s_Data->EntityClasses[sc.ClassName], entity);
			s_Data->EntityInstances[entityID] = instance;

			// Copy field values
			if (s_Data->EntityScriptFields.find(entityID) != s_Data->EntityScriptFields.end())
			{
				const ScriptFieldMap& fieldMap = s_Data->EntityScriptFields.at(entityID);
				for (const auto& [name, fieldInstance] : fieldMap)
					instance->SetFieldValueInternal(name, fieldInstance.m_Buffer);
			}

			instance->InvokeOnCreate();
		}
	}

	Scene* ScriptingEngine::GetSceneContext()
	{
		return s_Data->SceneContext;
	}

	void ScriptingEngine::OnUpdateEntity(Entity entity, TimeStep ts)
	{
		auto entityUUID = entity.GetUUID();
		if (s_Data->EntityInstances.find(entityUUID) != s_Data->EntityInstances.end())
		{
			Ref<ScriptInstance> instance = s_Data->EntityInstances[entityUUID];
			instance->InvokeOnUpdate((float)ts);
		}
		else
		{
			TSO_CORE_ERROR("Could not find ScriptInstance for entity {}", entityUUID);
		}
	}

	

	bool ScriptingEngine::LoadAssembly(const std::filesystem::path& filepath)
	{
		s_Data->AppDomain = mono_domain_create_appdomain("TsoScriptRuntime", nullptr);
		mono_domain_set(s_Data->AppDomain, true);

		s_Data->CoreAssemblyFilepath = filepath;
		s_Data->CoreAssembly = Utils::LoadMonoAssembly(filepath, false);
		if (s_Data->CoreAssembly == nullptr)
			return false;

		s_Data->CoreAssemblyImage = mono_assembly_get_image(s_Data->CoreAssembly);
		return true;
	}

	void ScriptingEngine::LoadAssemblyClasses()
	{
		s_Data->EntityClasses.clear();
		

		const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(s_Data->CoreAssemblyImage, MONO_TABLE_TYPEDEF);
		int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);
		MonoClass* entityClass = mono_class_from_name(s_Data->CoreAssemblyImage, "Tso", "Entity");

		for (int32_t i = 0; i < numTypes; i++)
		{
			uint32_t cols[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

			const char* nameSpace = mono_metadata_string_heap(s_Data->CoreAssemblyImage, cols[MONO_TYPEDEF_NAMESPACE]);
			const char* className = mono_metadata_string_heap(s_Data->CoreAssemblyImage, cols[MONO_TYPEDEF_NAME]);
			std::string fullName;
			if (strlen(nameSpace) != 0)
				fullName = fmt::format("{}.{}", nameSpace, className);
			else
				fullName = className;

			MonoClass* monoClass = mono_class_from_name(s_Data->CoreAssemblyImage, nameSpace, className);//TODO: make core and app devided

			if (monoClass == entityClass)
				continue;

			bool isEntity = mono_class_is_subclass_of(monoClass, entityClass, false);
			if (!isEntity)
				continue;

			Ref<ScriptClass> scriptClass = std::make_shared<ScriptClass>(nameSpace, className , true);
			s_Data->EntityClasses[fullName] = scriptClass;


			// This routine is an iterator routine for retrieving the fields in a class.
			// You must pass a gpointer that points to zero and is treated as an opaque handle
			// to iterate over all of the elements. When no more values are available, the return value is NULL.

			int fieldCount = mono_class_num_fields(monoClass);
			TSO_CORE_WARN("{} has {} fields:", className, fieldCount);
			void* iterator = nullptr;
			while (MonoClassField* field = mono_class_get_fields(monoClass, &iterator))
			{
				const char* fieldName = mono_field_get_name(field);
				uint32_t flags = mono_field_get_flags(field);
				if (flags & FIELD_ATTRIBUTE_PUBLIC)
				{
					MonoType* type = mono_field_get_type(field);
					ScriptFieldType fieldType = Utils::MonoTypeToScriptFieldType(type);
					//TSO_CORE_WARN("  {} ({})", fieldName, Utils::ScriptFieldTypeToString(fieldType));

					scriptClass->m_Fields[fieldName] = { fieldType, fieldName, field };
				}
			}

		}
		
	}

	MonoObject* ScriptingEngine::InstantiateClass(MonoClass* monoClass)
	{
		MonoObject* instance = mono_object_new(s_Data->AppDomain, monoClass);
		mono_runtime_object_init(instance);
		return instance;
	}

	bool ScriptingEngine::EntityClassExists(const std::string& className)
	{
		ScriptEngineData* test = s_Data;
		return s_Data->EntityClasses.find(className) != s_Data->EntityClasses.end();
	}

	void ScriptingEngine::OnScenePlay(Scene* context)
	{
		s_Data->SceneContext = context;
	}

	void ScriptingEngine::OnSceneStop()
	{
		s_Data->SceneContext = nullptr;
	}

	ScriptClass::ScriptClass(const std::string& classNamespace, const std::string& className, bool isCore)
		: m_ClassNamespace(classNamespace), m_ClassName(className)
	{
		m_MonoClass = mono_class_from_name(isCore ? s_Data->CoreAssemblyImage : s_Data->AppAssemblyImage, classNamespace.c_str(), className.c_str());
	}

	MonoObject* ScriptClass::Instantiate()
	{
		return ScriptingEngine::InstantiateClass(m_MonoClass);
	}

	MonoMethod* ScriptClass::GetMethod(const std::string& name, int parameterCount)
	{
		return mono_class_get_method_from_name(m_MonoClass, name.c_str(), parameterCount);
	}

	MonoObject* ScriptClass::InvokeMethod(MonoObject* instance, MonoMethod* method, void** params = nullptr)
	{
		MonoObject* exception = nullptr;
		return mono_runtime_invoke(method, instance, params, &exception);
	}


	ScriptInstance::ScriptInstance(Ref<ScriptClass> scriptClass, Entity entity)
		: m_ScriptClass(scriptClass)
	{
		m_Instance = scriptClass->Instantiate();

		m_Constructor = s_Data->EntityClass.GetMethod(".ctor", 1);
		m_OnCreateMethod = scriptClass->GetMethod("OnCreate", 0);
		m_OnUpdateMethod = scriptClass->GetMethod("OnUpdate", 1);

		// Call Entity constructor
		{
			auto entityID = entity.GetUUID();
			void* param = &entityID;
			m_ScriptClass->InvokeMethod(m_Instance, m_Constructor, &param);
		}
	}

	void ScriptInstance::InvokeOnCreate()
	{
		if (m_OnCreateMethod)
			m_ScriptClass->InvokeMethod(m_Instance, m_OnCreateMethod , nullptr);
	}

	void ScriptInstance::InvokeOnUpdate(float ts)
	{
		if (m_OnUpdateMethod)
		{
			void* param = &ts;
			m_ScriptClass->InvokeMethod(m_Instance, m_OnUpdateMethod, &param);
		}
	}

	bool ScriptInstance::GetFieldValueInternal(const std::string& name, void* buffer)
	{
		const auto& fields = m_ScriptClass->GetFields();
		auto it = fields.find(name);
		if (it == fields.end())
			return false;

		const ScriptField& field = it->second;
		mono_field_get_value(m_Instance, field.ClassField, buffer);
		return true;
	}

	bool ScriptInstance::SetFieldValueInternal(const std::string& name, const void* value)
	{
		const auto& fields = m_ScriptClass->GetFields();
		auto it = fields.find(name);
		if (it == fields.end())
			return false;

		const ScriptField& field = it->second;
		mono_field_set_value(m_Instance, field.ClassField, (void*)value);
		return true;
	}


}