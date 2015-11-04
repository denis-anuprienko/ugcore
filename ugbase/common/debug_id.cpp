
#include "debug_id.h"
#include "common/log.h"
#include "common/error.h"
#include "common/assert.h"
#include <string.h>
#include "util/string_util.h"

namespace ug{

/**
 * register the debug id.
 * NOTE: this function is called in the initialization of variables
 * of type DebugID, which are mostly global variables.
 * Be absolutely sure we are safe here, i.e.
 * we are not using other things which might not be initialized (like Log).
 */
DebugID::DebugID(const char *str)
{
	m_hash = crc32(str);
	GetDebugIDManager().register_debug_id(str);
}

DebugIDManager& DebugIDManager::instance()
{
	static DebugIDManager m;
	return m;
}


bool DebugIDManager::
set_debug_levels(int lev)
{
	for(std::map<uint32, int>::iterator it=m_dbgLevels.begin(); it != m_dbgLevels.end(); ++it)
		(*it).second = lev;
	return true;
}

bool DebugIDManager::
set_debug_level(const char *debugID, int level)
{
	int slen = strlen(debugID);
	if(slen<=0) return false;
	if(debugID[slen-1] == '*')
	{
		for(size_t i=0; i<m_dbgLevelIdentifiers.size(); i++)
		{
			const char *name = m_dbgLevelIdentifiers[i].c_str();
			if(WildcardMatch(name, debugID))
			{
				set_debug_level(crc32(name), level);
				//UG_LOGN(name);
			}
		}
	}
	else if(set_debug_level(crc32(debugID), level) == false)
	{
		UG_LOG("DebugID " << debugID << " not registered.\n");
		return false;
	}
	return true;
}

/**
 * register the debug id.
 * NOTE: this function is called in the initialization of global variables
 * of type DebugID. Be absolutely sure we are safe here, i.e.
 * we are not using other things which might not be initialized (like Log).
 */
bool DebugIDManager::
register_debug_id(const char *debugID)
{
	if(debug_id_registered(debugID) == false)
	{
		m_dbgLevelIdentifiers.push_back(std::string(debugID));
		m_dbgLevels[crc32(debugID)] = -1;
		return true;
	}
	else
	{
		// not quite clear if cout is defined yet.
		// note that this could be caused by double-registering libraries.
		UG_THROW("FATAL ERROR: DebugID "<<debugID<<" already registered.");
		return false;
	}
}

}
