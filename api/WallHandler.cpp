#include "WallHandler.h"

#include "Operators.h"
#include "ApiTypes.h"

#include "rswall.h"
#include <retroshare/rsidentity.h>
#include <util/radix64.h>
#include "GxsResponseTask.h"

#include <algorithm>
#include <unistd.h>

namespace resource_api
{

//using namespace RsWall;

class WallTask: public GxsResponseTask
{
public:
    WallTask(RsWall::RsWall* wall, RsIdentity* identities, RsGxsId id):
        GxsResponseTask(identities, wall->getTokenService()),
        mWall(wall), mId(id), mState(BEGIN){}
private:
    RsWall::RsWall* mWall;
    RsGxsId mId;
    enum State {BEGIN, WAITING};
    State mState;
    uint32_t mToken;
protected:
    virtual void gxsDoWork(Request &req, Response &resp)
    {
        switch(mState)
        {
        case BEGIN:
        {
            mWall->requestWallGroups(mToken, mId);
            addWaitingToken(mToken);
            requestGxsId(mId);
            mState = WAITING;
            break;
        }
        case WAITING:
        {
            std::vector<RsWall::WallGroup> wgs;
            mWall->getWallGroups(mToken, wgs);
            if(!wgs.empty())
            {
                // for now only the first group
                RsWall::WallGroup& wg = wgs.front();

                // make base63 string out of images
                // TODO: find and implement better solution
                // TODO: use correct mime type, don't assume jpeg
                std::string wi_b64;
                Radix64::encode((const char *)wg.mWallImage.mData.data(),wg.mWallImage.mData.size(),wi_b64);
                wi_b64 = "data:image/jpeg;base64," + wi_b64;

                std::string ai_b64;
                Radix64::encode((const char *)wg.mAvatarImage.mData.data(),wg.mAvatarImage.mData.size(),ai_b64);
                ai_b64 = "data:image/jpeg;base64," + ai_b64;

                streamGxsId(mId, resp.mDataStream.getStreamToMember("identity"));
                resp.mDataStream << makeKeyValueReference("profile_text", wg.mProfileText)
                                 << makeKeyValueReference("wall_image", wi_b64)
                                 << makeKeyValueReference("avatar_image", ai_b64);
                resp.mReturnCode = Response::OK;
            }
            else
            {
                resp.mReturnCode = Response::FAIL;
                resp.mDebug << "Error: no wall group" << std::endl;
            }
            done();
            break;
        }
        }
    }
};

bool waitForTokenOrTimeout(uint32_t token, RsTokenService* tokenService)
{
    time_t start = time(NULL);
    while(   (tokenService->requestStatus(token) != RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
          && (tokenService->requestStatus(token) != RsTokenService::GXS_REQUEST_V2_STATUS_FAILED)
          && (time(NULL) < (start+10))
          )
    {
#ifdef WINDOWS_SYS
        Sleep(500);
#else
        usleep(500*1000) ;
#endif
    }
    if(tokenService->requestStatus(token) == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

WallHandler::WallHandler(RsWall::RsWall* wall, RsIdentity* identity):
    mRsWall(wall), mRsIdentity(identity)
{
    addResourceHandler("*", this, &WallHandler::handleWildcard);
    addResourceHandler("own", this, &WallHandler::handleOwn);
    addResourceHandler("activities", this, &WallHandler::handleActivities);
    addResourceHandler("wall", this, &WallHandler::handleWall);
    addResourceHandler("avatar_image", this, &WallHandler::handleAvatarImage);
}

std::string WallHandler::help()
{
    return
            "GET /activities\n"
            "return a list of the current activities\n"
            "\n"
;
}

ResponseTask* WallHandler::handleWildcard(Request &req, Response &resp)
{
    bool ok = true;
    ResponseTask* ret = 0;
    if(req.mMethod == Request::GET && !req.mPath.empty())
    {
        RsGxsId id(req.mPath.top());
        if(!id.isNull())
        {
            ret = new WallTask(mRsWall, mRsIdentity, id);
        }
        else ok = false;
    }
    else
    {
    }

    if(ok)
    {
        resp.mReturnCode = Response::OK;
    }
    else
    {
        resp.mReturnCode = Response::FAIL;
    }
    return ret;
}

void WallHandler::handleOwn(Request &req, Response &resp)
{
    bool ok = true;

    if(ok)
    {
        resp.mReturnCode = Response::OK;
    }
    else
    {
        resp.mReturnCode = Response::FAIL;
    }
}

// this function is the absolute horror
// it pulls many resources together
// this means the function has to fetch information in many little pieces from different places
// it goes like this.
// activity contains gxs-ids and a gxs-grp-id
//   fetch the names for the gxs-ids
//   fetch the grp meta for the gxs-grp-id
//     fetch the name for the author id in meta
//   fetch the post msg in the grp with the gxs-grp-id
//     fetch the name of the author id
//
// so this function is an example of how it should not be
// many different services have to fetch for example gxs-ids
// need a generic solution to fill in the names for gxs-ids
//
// idea: instead of the resource itself, add a link to the resource to the stream
// a resource resolver would then go through the stream until all links are replaced with their values
// better idea: other handlers like identity handler expose a funktion to write the identity details to a stream
void WallHandler::handleActivities(Request &req, Response &resp)
{
    std::vector<RsWall::Activity> activities;
    mRsWall->getCurrentActivities(activities);

    // collect ids of referenced post grps
    std::list<RsGxsGroupId> postGrpIds;
    for(std::vector<RsWall::Activity>::iterator vit = activities.begin(); vit != activities.end(); vit++)
    {
        RsWall::Activity& activity = *vit;
        postGrpIds.push_back(activity.mReferencedGroup);
    }

    // request post grps
    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;
    uint32_t token;
    mRsWall->getTokenService()->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, postGrpIds);

    // wait for request completion
    time_t start = time(NULL);
    while((mRsWall->getTokenService()->requestStatus(token) != RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
          &&(mRsWall->getTokenService()->requestStatus(token) != RsTokenService::GXS_REQUEST_V2_STATUS_FAILED)
          &&((time(NULL) < (start+10)))
          )
    {
#ifdef WINDOWS_SYS
        Sleep(500);
#else
        usleep(500*1000) ;
#endif
    }

    std::list<RsGroupMetaData> postGrpMetas;
    mRsWall->getGroupSummary(token, postGrpMetas);

    // request the msgs
    std::vector<uint32_t> postMsgTokens;
    for(std::vector<RsWall::Activity>::iterator vit = activities.begin(); vit != activities.end(); vit++)
    {
        RsWall::Activity& activity = *vit;

        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
        uint32_t token;
        std::list<RsGxsGroupId> grpIdList;
        grpIdList.push_back(activity.mReferencedGroup);
        mRsWall->getTokenService()->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, grpIdList);
        postMsgTokens.push_back(token);
    }

    // wait for post msgs
    bool ready = false;
    time_t start2 = time(NULL);
    while(!ready && (time(NULL)<(start2+10)))
    {
        ready = true;
        for(std::vector<uint32_t>::iterator vit = postMsgTokens.begin(); vit != postMsgTokens.end(); vit++)
        {
            ready &= (mRsWall->getTokenService()->requestStatus(*vit) == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
                    ||(mRsWall->getTokenService()->requestStatus(*vit) == RsTokenService::GXS_REQUEST_V2_STATUS_FAILED);
        }
#ifdef WINDOWS_SYS
        Sleep(500);
#else
        usleep(500*1000) ;
#endif
    }

    // get the msgs
    // it could happend that a mesage is not available
    // maybe should handle this
    std::vector<RsWall::PostMsg> postMsgs;
    for(std::vector<uint32_t>::iterator vit = postMsgTokens.begin(); vit != postMsgTokens.end(); vit++)
    {
        RsWall::PostMsg pm;
        mRsWall->getPostMsg(*vit, pm);
        postMsgs.push_back(pm);
    }

    // collect the involved author ids to get their names
    std::map<RsGxsId, std::string> authors;
    for(std::vector<RsWall::Activity>::iterator vit = activities.begin(); vit != activities.end(); vit++)
    {
        RsWall::Activity& activity = *vit;
        for(std::vector<RsGxsId>::iterator vit = activity.mShared.begin(); vit != activity.mShared.end(); vit++)
        {
            authors[*vit] = "";
        }
        for(std::vector<RsGxsId>::iterator vit = activity.mCommented.begin(); vit != activity.mCommented.end(); vit++)
        {
            authors[*vit] = "";
        }
    }
    for(std::list<RsGroupMetaData>::iterator lit = postGrpMetas.begin(); lit != postGrpMetas.end(); lit++)
    {
        RsGroupMetaData& meta = *lit;
        authors[meta.mAuthorId] = "";
    }

    // now fetch the names
    time_t start3 = time(NULL);
    bool allDone = false;
    while(!allDone && (time(NULL)<(start3+10)))
    {
        allDone = true;
        for(std::map<RsGxsId, std::string>::iterator mit = authors.begin(); mit != authors.end(); mit++)
        {
            if(mit->second == "")
            {
                RsIdentityDetails details;
                if(mRsIdentity->getIdDetails(mit->first, details))
                {
                    mit->second = details.mNickname;
                }
                else
                {
                    allDone = false;
                }
            }
        }
    }

    // put everything together in one response
    for(std::vector<RsWall::Activity>::iterator vit = activities.begin(); vit != activities.end(); vit++)
    {
        StreamBase& stream = resp.mDataStream.getStreamToMember();
        RsWall::Activity& activity = *vit;
        StreamBase& stream_shared = stream.getStreamToMember("names_shared");
        for(std::vector<RsGxsId>::iterator vit = activity.mShared.begin(); vit != activity.mShared.end(); vit++)
        {
            stream_shared << makeValueReference(authors[*vit]);
        }
        StreamBase& stream_commented = stream.getStreamToMember("names_commented");
        for(std::vector<RsGxsId>::iterator vit = activity.mCommented.begin(); vit != activity.mCommented.end(); vit++)
        {
            stream_commented << makeValueReference(authors[*vit]);
        }

        RsGroupMetaData postGrpMeta;
        for(std::list<RsGroupMetaData>::iterator postGrpMetaIt = postGrpMetas.begin(); postGrpMetaIt != postGrpMetas.end(); postGrpMetaIt++)
        {
            if(postGrpMetaIt->mGroupId == activity.mReferencedGroup)
            {
                postGrpMeta = *postGrpMetaIt;
            }
        }
        RsWall::PostMsg pm;
        for(std::vector<RsWall::PostMsg>::iterator pmvit = postMsgs.begin(); pmvit != postMsgs.end(); pmvit++)
        {
            RsWall::PostMsg& pmref = *pmvit;
            if(pmref.mMeta.mGroupId == activity.mReferencedGroup)
            {
                pm = pmref;
            }
        }
        StreamBase& stream_post = stream.getStreamToMember("post");
        stream_post << makeKeyValueReference("author", authors[postGrpMeta.mAuthorId])
                    << makeKeyValueReference("post_text", pm.mPostText);


    }
}

void WallHandler::handleWall(Request &req, Response &resp)
{
    if(!req.mPath.empty())
    {
        RsGxsId author(req.mPath.top());
        uint32_t token;
        // todo
        //mRsWall->requestWallGroups();
    }
    else
    {
        // list walls
        uint32_t token;
        mRsWall->requestWallGroupMetas(token, RsGxsId());

        waitForTokenOrTimeout(token, mRsWall->getTokenService());

        std::vector<RsGroupMetaData> metas;
        mRsWall->getWallGroupMetas(token, metas);

        //
        for(std::vector<RsGroupMetaData>::iterator vit = metas.begin(); vit != metas.end(); vit++)
        {
            RsGroupMetaData& meta = *vit;
            StreamBase& stream = resp.mDataStream.getStreamToMember();

            KeyValueReference<RsGxsGroupId> wallId("wall_id", meta.mGroupId);
            KeyValueReference<RsGxsCircleId> circleId("todo_circle_id", meta.mCircleId);
            stream << wallId << circleId;

            time_t start = time(NULL);
            // have to try to get the identity details multiple times, until they are cached
            bool ok  = false;
            while(!ok && (time(NULL)< (start+10)))
            {
                RsIdentityDetails details;
                // this returns cached data and false if the data is not yet cached
                ok = mRsIdentity->getIdDetails(meta.mAuthorId, details);
                if(ok)
                {
                    KeyValueReference<RsGxsId> id("id", details.mId);
                    KeyValueReference<RsPgpId> pgp_id("pgp_id", details.mPgpId);
                    std::string avatar_address = "/../avatar_image/"+meta.mAuthorId.toStdString();
                    stream.getStreamToMember("author")
                            << makeKeyValueReference("name", details.mNickname)
                            << id
                            << pgp_id
                            << makeKeyValueReference("pgp_linked", details.mPgpLinked)
                            << makeKeyValueReference("own", details.mIsOwnId)
                            << makeKeyValue("avatar_address", avatar_address)
                               ;
                }
#ifdef WINDOWS_SYS
                Sleep(500);
#else
                usleep(500*1000) ;
#endif
            }
        }
    }
}

void WallHandler::handleAvatarImage(Request &req, Response &resp)
{
    if(!req.mPath.empty())
    {
        RsGxsId author(req.mPath.top());

        uint32_t token;
        mRsWall->requestWallGroups(token, author);

        waitForTokenOrTimeout(token, mRsWall->getTokenService());

        std::vector<RsWall::WallGroup> wgs;
        mRsWall->getWallGroups(token, wgs);

        if(!wgs.empty())
        {
            RsWall::WallGroup& grp = wgs.front();
            resp.mDataStream << grp.mAvatarImage.mData;
        }
    }
}

} // namespace resource_api
