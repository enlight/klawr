// includes auto-generated headers from the Klawr.ClrHost.Managed type library that's generated
// from the Klawr.ClrHost.Managed assembly

#ifdef _DEBUG
#import "../../ClrHostManaged/bin/Debug/Klawr.ClrHost.Managed.tlb"
#else
#import "../../ClrHostManaged/bin/Release/Klawr.ClrHost.Managed.tlb"
#endif // _DEBUG

// Unfortunately the rename_namespace attribute of the import directive doesn't support nested
// namespaces, so to get imported types into a nested namespace we have to pull them in using 
// type aliases.
namespace Klawr
{
	namespace Managed
	{
		using IDefaultAppDomainManager = Klawr_ClrHost_Managed::IDefaultAppDomainManager;
		using IDefaultAppDomainManagerPtr = Klawr_ClrHost_Managed::IDefaultAppDomainManagerPtr;
		using IEngineAppDomainManager = Klawr_ClrHost_Managed::IEngineAppDomainManager;
		using IEngineAppDomainManagerPtr = Klawr_ClrHost_Managed::IEngineAppDomainManagerPtr;
		
		using ObjectUtilsProxy = Klawr_ClrHost_Managed::ObjectUtilsProxy;
		using LogUtilsProxy = Klawr_ClrHost_Managed::LogUtilsProxy;

		using ScriptComponentProxy = Klawr_ClrHost_Managed::ScriptComponentProxy;
		using ScriptObjectInstanceInfo = Klawr_ClrHost_Managed::ScriptObjectInstanceInfo;
	} // namespace Managed
} // namespace Klawr
