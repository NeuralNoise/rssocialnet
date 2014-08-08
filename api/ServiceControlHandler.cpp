#include "ServiceControlHandler.h"

#include "retroshare/rsservicecontrol.h"

#include "Operators.h"

namespace resource_api
{
// maybe move to another place later
// need more generic operators for list, vector, map
template<class T>
void setToStream(StreamBase& stream, std::set<T>& set)
{
    if(stream.serialise())
    {
        for(typename std::set<T>::iterator sit = set.begin(); sit != set.end(); sit++)
        {
            T item = *sit;
            stream << makeValueReference(item);
        }
    }
    else
    {
        while(stream.hasMore())
        {
            T item;
            stream << makeValueReference(item);
            set.insert(item);
        }
    }
}

void servicePermissionToStream(StreamBase& stream, RsServicePermissions& perm)
{
    stream << makeKeyValueReference("service_id", perm.mServiceId)
           << makeKeyValueReference("service_name", perm.mServiceName)
           << makeKeyValueReference("default_allowed", perm.mDefaultAllowed)
              ;
    setToStream(stream.getStreamToMember("peers_allowed"), perm.mPeersAllowed);
    setToStream(stream.getStreamToMember("peers_denied"), perm.mPeersDenied);
}

ServiceControlHandler::ServiceControlHandler(RsServiceControl* control):
    mRsServiceControl(control)
{
    addResourceHandler("*", this, &ServiceControlHandler::handleWildcard);
}

std::string ServiceControlHandler::help()
{
    return  "GET /\n"
            "return array of service info objects\n"
            ;
}

void ServiceControlHandler::handleWildcard(Request &req, Response &resp)
{
    bool ok = false;
    if(!req.mPath.empty())
    {
    }
    else
    {
        // no more path element
        if(req.mMethod == Request::GET)
        {
            // list all peers
            ok = true;
            RsPeerServiceInfo psi;
            ok &= mRsServiceControl->getOwnServices(psi);
            for(std::map<uint32_t, RsServiceInfo>::iterator mit = psi.mServiceList.begin(); mit != psi.mServiceList.end(); mit++)
            {
                RsServicePermissions perms;
                ok &= mRsServiceControl->getServicePermissions(mit->first, perms);
                if(ok)
                {
                    servicePermissionToStream(resp.mStream.getStreamToMember(), perms);
                }
            }
        }
        else if(req.mMethod == Request::POST)
        {

        }
    }
    if(ok)
    {
        resp.mReturnCode = 0;
    }
    else
    {
        resp.mReturnCode = 1;
    }
}

} // namespace resource_api
