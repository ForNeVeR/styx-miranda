#include "PluginCore.h"

#include <memory>
#include <string>

#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>

#include <m_clist.h>
#include <m_database.h>
#include <m_langpack.h>
#include <m_skin.h>

#include "Connector.h"
#include "MemoryUtils.h"
#include "MessageFactory.h"
#include "MirandaContact.h"
#include "StringUtils.h"

using namespace ru::org::codingteam::styx;

const auto EnableServiceName = "Styx/EnableCommand";

PluginCore::PluginCore(PLUGININFOEX &pluginInfo)
	: _pluginInfo(pluginInfo), _isSynchronizationEnabled(false), _connector(new Connector())
{
}

PluginCore::~PluginCore()
{
	DisableSynchronization();
}

void PluginCore::Initialize()
{
	boost::log::add_file_log("styx.log");
	BOOST_LOG_TRIVIAL(info) << "Initialized";

	InitializeLangpack();
	InitializeMainMenu();
	InitializeHooks();
}

void PluginCore::Deinitialize()
{
	DisableSynchronization();
}

void PluginCore::EnableSynchronization()
{
	if (!_isSynchronizationEnabled)
	{
		_connector->start();
		_isSynchronizationEnabled = true;
	}
}

void PluginCore::DisableSynchronization()
{
	if (_isSynchronizationEnabled)
	{
		_connector->stop();
		_isSynchronizationEnabled = false;
	}
}

void PluginCore::InitializeLangpack()
{
	mir_getLP(&_pluginInfo);
}

void PluginCore::InitializeMainMenu()
{
	auto enableService = [](void *corePointer, LPARAM param1, LPARAM param2) -> INT_PTR
	{
		auto core = static_cast<PluginCore*>(corePointer);
		core->EnableSynchronization();
		return 0;
	};

	CreateServiceFunctionObj(EnableServiceName, enableService, this);

	auto mi = CLISTMENUITEM();
	mi.cbSize = sizeof(mi);
	mi.position = -0x7FFFFFFF;
	mi.flags = CMIF_UNICODE;
	mi.hIcon = LoadSkinnedIcon(SKINICON_OTHER_MIRANDA);
	mi.ptszName = LPGENT("&Enable Styx");
	mi.pszService = const_cast<char*>(EnableServiceName);

	Menu_AddMainMenuItem(&mi);
}

void PluginCore::InitializeHooks()
{
	auto onEventAdded = [](void *corePointer, WPARAM wParam, LPARAM lParam) -> int
	{
		auto core = static_cast<PluginCore*>(corePointer);
		auto contactHandle = reinterpret_cast<HANDLE>(wParam);
		auto dbEventHandle = reinterpret_cast<HANDLE>(lParam);

		auto message = MessageFactory::fromMirandaHandles(contactHandle, dbEventHandle);
		if (!message)
		{
			return 0;
		}

		core->_connector->queueMessage(message.get());

		return 0;
	};

	HookEventObj(ME_DB_EVENT_ADDED, onEventAdded, this);
}