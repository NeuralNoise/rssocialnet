#pragma once

#include <retroshare/rsgxsifacehelper.h>
#include <retroshare/rsgxscommon.h>

// who cleans up old tokens generated by the dataservice?
// maybe they expire, but i can't find where they get removed
// they can't expire, because dataaccess has only a map<token, status>

/* the life of a token
static const uint8_t GXS_REQUEST_V2_STATUS_FAILED;      end if the request failed
static const uint8_t GXS_REQUEST_V2_STATUS_PENDING;     token was just created
static const uint8_t GXS_REQUEST_V2_STATUS_PARTIAL;
static const uint8_t GXS_REQUEST_V2_STATUS_FINISHED_INCOMPLETE;     unused
static const uint8_t GXS_REQUEST_V2_STATUS_COMPLETE;    the request was completed and data is available
static const uint8_t GXS_REQUEST_V2_STATUS_DONE;        the client received the data

*/

// ways of getting data
// - blocking function gets data and returns it immediately
// - non blocking function returns data if available, else it returns false
//   call the function again to get data
//   this works only if the data gets cached for a while,
//     because the service can't know when the client received it
// - a first function takes arguments and returns a token
//   the token has to be checked if the requested data is available
//   another function returns the data

// big question:
// a fb page is ver similar to a rs-channel
// would be nice to display channel msgs in the webui
// or display the social network things in the Qt ui
// idea: import content using the feedreader

// can configure in netservice, if groups should be auto synced
// if not can request which groups should be synced, but need peer id for this

// need some flags
// it is desired to have flags in gxs-meta, because this is cheaper to load than full msgs
// and if using available flags, rsgxsdataaccess can do the filtering,
// thus don't have to write own code for filtering
//
// in gxs-grp-meta
//   meta->mGroupStatus = GXS_SERV::GXS_GRP_STATUS_UNPROCESSED | GXS_SERV::GXS_GRP_STATUS_UNREAD;
//   meta->mSubscribeFlags = GXS_SERV::GROUP_SUBSCRIBE_NOT_SUBSCRIBED;
//   grp->metaData->mSubscribeFlags = GXS_SERV::GROUP_SUBSCRIBE_ADMIN
//                                  | GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED | GXS_SERV::GROUP_SUBSCRIBE_PUBLISH;
//   std::string mServiceString; can be used to store whatever we like
//   uint32_t mAuthenFlags;     unused?
//   RsGxsGroupId mParentGrpId; unused?
//   uint32_t    mSignFlags;    unused?
//   uint32_t    mGroupFlags;   used for three auth types private, restricted, public
//   std::string mGroupName;    could abuse this
//
// in gxs-msg-meta
//      three members to point to other messages: (or can abuse this and store own hash of whatever is there)
//          RsGxsMessageId mThreadId;
//          RsGxsMessageId mParentId;
//          RsGxsMessageId mOrigMsgId;
//
//      /// the first 16 bits for service, last 16 for GXS
// question: what means first bits? lower or upper?
// because photoservice and wiki uses lower bits
// and in mMsgStatus gxs uses lower bits
//      uint32_t    mMsgFlags;
//
//      // BELOW HERE IS LOCAL DATA, THAT IS NOT FROM MSG.
//      // normally READ / UNREAD flags. LOCAL Data.
//
//      /// the first 16 bits for service, last 16 for GXS
//      uint32_t    mMsgStatus;
//          use gxs part of msg status like this:
//          meta->mMsgStatus = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED | GXS_SERV::GXS_MSG_STATUS_UNREAD;
//
//      time_t      mChildTs;
//      std::string mServiceString; // Service Specific Free-Form extra storage.



/*
clean plan
==========

// ************ UI classes **********************
class TokenQueueWt
class UpdateBroadcastWt

class WallWidget
class NewsfeedWidget
class NewPostWidget
class RootPostWidget
class AvatarImageWidget
class WallCommentContainerWidget
class NewCommentWidget
class CommentWidget
class ActionButtonWidget // like, share,

first run pages

later
-----
distant mail
distant chat
chatlobbies

*/
// todo
class NewsfeedEntry;

class RsWall;
class Image;
class WallGroup;
class PostGroup;
class ReferenceMsg;
class PostMsg;

extern RsWall *rsWall;

// ********** interface *************************
class RsWall: public RsGxsIfaceHelper
{
public:
    RsWall(RsGxsIface *gxs)
    :RsGxsIfaceHelper(gxs)  { return; }

    // important: newsfeed handling
    // the ui has to ask if the newsfeed has new entries
    //   what is NewsfeedEntry? probably a pair (RsGxsGroupId,RsGxsMsgId)
    //   RsGxsMsgId can be null to signal that only the RsGxsGroupId is important for the newsfeed
    //     have to provide type info here, so the ui knows which fn to use to retrieve data
    virtual void getNewNewsfeedEntries(std::list<NewsfeedEntry> &feeds) = 0;
    // the Retroshare newsfeed has no state across start/stop of retroshare
    // do we need to save a newsfeed state to disk?

    virtual void createWallGroup(uint32_t &token, const WallGroup &grp) = 0;
    virtual void updateWallGroup(uint32_t &token, const WallGroup &grp) = 0;

    // how to handle selection of target wall?
    //   maybe we have different walls private/public
    // who fills in the cirlce id?
    //   probably the service, because he has time and knowledge, and can cache things
    virtual void createPost(uint32_t &token, const PostMsg &msg) = 0;
virtual void acknowledgeCreatePost(uint32_t &token) = 0;

    // this handles sharing on own or friends wall, and like
    virtual void createPostReferenceMsg(uint32_t &token, const ReferenceMsg &refMsg) = 0;

    // use this if you only know the identity of the wall owner
    // use request in gxsifacehelper if you want to list all known walls
    // (this will be expensive, because have to request all gxs-groups)
    // (better have a flag in meta, so have to request meta only?)
    // (would be cool if gxs could filter by RsItem-type)
    // maybe should return a list of walls, because of public wall and private wall?
    //   ->merge results from both walls
    virtual void requestWallGroupMetas(uint32_t &token, const RsGxsId &identity) = 0;
    virtual void getWallGroupMetas(const uint32_t &token, std::vector<RsGroupMetaData>& grpMeta) = 0;
    virtual void requestWallGroups(uint32_t &token, const RsGxsId &identity) = 0;
    virtual void getWallGroups(const uint32_t &token, std::vector<WallGroup> &wgs) = 0;

    virtual void getPostGroup(const uint32_t &token, PostGroup &pg) = 0;
    virtual void getPostReferenceMsg(const uint32_t &token, ReferenceMsg &refMsg) = 0;
    virtual void getPostMsg(const uint32_t &token, PostMsg &pm) = 0;

    virtual void requestAvatarImage(uint32_t &token, const RsGxsId &identity) = 0;
    virtual bool getAvatarImage(const uint32_t &token, Image &image) = 0;

    // functions for comment service
    // (just a forward to p3GxsCommentService like in p3GxsChannels)
};

/*

// ************* Wall Service ******************
// have two threads: ui thread, rsgenexchange thread
// have to be careful if they cross their ways
class p3WallService: public RsWall, public RsGenExchange, public GxsTokenQueue{
public:
    // *********** begin RsWall ***********
    // wall iface here
    // ************ end RsWall ************

    // from RsGenExchange
    // this is called by RsGenExchange
    virtual void notifyChanges(std::vector<RsGxsNotify*>& changes);

    // from RsGenExchange
    // this is called by RsGenExchange runner thread
    virtual void service_tick();

    // from RsGenExchange
    virtual RsServiceInfo getServiceInfo();

    // from GxsTokenQueue
    virtual void handleResponse(uint32_t token, uint32_t req_type);

private:
    // check if we want to subscribe to incoming groups
    // detect interesting group-ids in messages
    virtual void _checkSubscribe(std::vector<RsGxsNotify*>& changes);

    // set to collect wanted groups
    std::set<RsGxsGroupId> _mGroupsToSubscribe;

    // collect info about incoming groups and messages
    // generate newsfeed entries from it
    // first this will just record interesting things by by timestamp
    // later we maybe want a filter with for example 5 mins delay
    // and then rate all messages in this period and filter some out
    // lets see how much content a user receives,
    // if its to much think about filters
    // the user probably wants to tune the filter
    virtual void _filterNews(std::vector<RsGxsNotify*>& changes);
};
*/

// **************** data classes **********************
// want to check if the file type is on a white list?
// http://en.wikipedia.org/wiki/List_of_file_signatures
// want to avoid disaster because of handing untrusted data to the browser
// svg scared me

// RsGxsImage is to complicated
// this class is much simpler, because the memory managment is hidden in a std::vector
class Image{
public:
    std::vector<uint8_t> mData;
};

class WallGroup{
public:
    RsGroupMetaData mMeta;
    // in meta:
    // mAuthorId (required)
    // mCirlceId (optional)
    std::string mProfileText;
    Image mAvatarImage;
    Image mWallImage;
};
class PostGroup{
public:
    RsGroupMetaData mMeta;
    // in meta:
    // mAuthorId (optional)
    // mCirlceId (optional)
    // want to have a label to know type of content in PostGroup?
    // could put a snippet of the content here
    // this would allow to have it available without beeing subscribed
};
class ReferenceMsg{
public:
    RsMsgMetaData mMeta;
    // in meta:
    // mAuthorId (optional)
    RsGxsGroupId mReferencedGroup; // this can be a id of a PostGroup or WallGroup
                                   // or even id of a photo album

    // different use case for references:
    // - share post on wall
    // - like post
    // - share wall
    // - like wall
    uint32_t mType;

    // idea:
    // uint32_t mReferencedServiceType
    // this would allow to link in posts from other gxs services
};
class PostMsg{
public:
    RsMsgMetaData mMeta;
    // in meta:
    // mGroupId
    // mAuthorId (optional, but same as in PostGroup)

    // want to allow different content for posts:
    // - text
    // - images
    // - a web url
    // - hash of a file
    std::string mPostText;
    //std::vector<Image> mImages; // gxs-msgs are limited in size, so maybe only one image
                                // (use photo albums for image sets)
    // not in serialiser yet
    //Image mImage;
    //std::vector<RsGxsFile> mFiles; // FileMeta is hash + size + name + (some other meta maybe?)
};

/*
// *************** item classes ***********************
class WallGroupItem: public RsGxsGrpItem
{
public:
    WallGroup mWallGroup;
}
class PostGroupItem: public RsGxsGrpItem
{
public:
    PostGroup mPostGroup;
}

class ReferenceMsgItem: public RsGxsMsgItem
{
public:
    ReferenceMsg mReferenceMsg;
}
class PostMsgItem: public RsGxsMsgItem
{
public:
    PostMsg mPostMsg;
}

// also used:
// RsGxsCommentItem
// RsGxsVoteItem

// ************ end clean plan **********************
*/



// how to process incoming grps and msgs?
// grps
//   check if grp id is in wanted grps list, then subscribe
// msgs
//   check if referenced grp is subscribed or available
//     if available: subscribe
//     if not available: put into wanted grps list
// if a new post-grp was subscribed:
//   make newsfeed entry
//
// have to track wall grp-ids
// have to track circles of friends
// wall-grps are easy to track, because they are of type wall-grp and have author id set
//   can then derive friends groups from circle ids in wall grps
//
// how to maintain newsfeed?
//   sort by time of arrival
//   want to re-sort if new comments or votes arrive?
//   make placeholder function to allow filtering of newsfeed entrys later

// problem: can't browse gxs-grp-content without subscribing to it
//   rs is missing a load-on-demand function to browse content
// but can subscribe to group and subscribe and download only the latest post-grps
// group stores 365 days * 10 referenced posts/day * 500 bytes/referenced post
//   = 1,8 megabytes
// this allows to subscribe to all interesting groups and then pick latest posts
//   this would lead to not all posts available, just the posts at the time the site was interesting
//     can live with this, if the site was really interesting neighbor users would subscribe and download all posts
// subscribe all posts if the user likes the site
// if i learn about 10 sites per day i have to download 18 megabytes per day
// assuming everyone has 400 kbit/s = 50 kbytes/s upstream
//   then have to upload for 6mins
//     this is ok, even if i have to do this 2-3 times it is ok

// dsl rates
// 1024/128
// 2048/192
// 6016/512

// if we store friendslist as a circle
// then we need to edit circles
// how can someone notice if he gets removed from a circle?
// impossible for self restricted circle, because he can't receive the circle anymore
//   idea: add a list of to-be-removed-ids to the circle
//     then first add to remove list and later remove from allowed list
//     the last thing the removed one can see is its own name on the removed list
//     this does not work, because the latest circle-info is used for permission check

// subcircles would allow to send msgs to friends + friends of friends
// future, if the complicated subcircle things is available
// how fof circle would work:
//   everyone has a circle for friends
//   and a circle for fof, which consists of subcricles which are the own and friends friends circles

// plan:
// wall -> gxs-group
//   connected to a single identity, so this is a gxs-group with author
//   msgs are just group-ids of posts
//   these ids can even be cross-service ids
//     this would allow to have all sort of content on the wall, like a link to a photo album
//       but which service would handle the comments? the wall service?
//   can have different groups:
//     one for friends
//     one for the public
//   if i like something: publish it in wall grp
//   if i share something, make msg with grp-id of content
//   others can make msgs with grp-ids of content
//   subscribe by friendship(automated) or follow
//   who can post?
//   public wall: everyone
//   friends only wall: friends only. this is a implicit rule,
//     because you have to see the gxs-grp to make posts in it
//   don't need a publish sign, because posts are signed by author which is same as grp author in own wall-grp
//   what about a profile?
//     store basic profile info in grp item to have it available without subscribing?
//       i can see comments/posts from fofs on my friends wall
//       i have the fof wall grp item from my friends, but i'm not subscribed
//       so if profile data is stored in grp, then i could see profile of fof
//
// site -> gxs-group
//   msgs contain grp-ids of posts
//   similar to wall
//   not connected to a single identity
//   a site is a public space
//   subscribe by hand
//     can't see content without subscribing
//     two subscribe levels: auto subscribe to gxs-grps which could be interesting
//     and have another user layer subscribe to let the user choose if it is interesting
//     have also to subscribe to content-grps
//     don't want to download all content just because it could be interesting -> problem
//   what about author and publish sign?
//
// root post -> gxs-group
//   gxs-group can have author, but it can't be signed by publish key of other gxs-grp
//      would need a parent grp label which gets signed by parent grps publish key
//      similar to identity system
//        this would not be needed, if it was possible to sign single messages in a grp with the publish key
//        then the root post msgs in the root post grp would be signed by publish
//   so to verify if a gxs-grp as root post belongs to another grp we have check if it is referenced in this grp
//      but then this has to be a grp with publish sign
//   can also have a anon root post = anon gxs-grp
//
// gxs offers three auth/sign levels:
//   public
//   restricted
//   private     make this to require publish sign
// probably always want author sign
//
// about blocking unwanted content
//   want to remove content from wall or pages
//   can publish a signed delete msg
//   good receivers should then unsubscribe from the gxs-grps hosting the content
//     unsubscribed gxs-grps can be garbage collected at some time in the future

// notes:
// ------
// how does gxs reputation work? where does mPeerOpinion come from?
//   probably from reputation service
// can delete gxs-group??? how does this work? there is no tombstone, so the group info will we received again?
// can't delete gxs-msg
// libretroshare has a tokenqueue: gxstokenqueue
// can get a token for own use from rsgenexchange
//    cool, because this allows us to define own requests which consist of different requestst
// unused?: mSignFlags
// same #defines in rstokenservice.h and rsgxsdataaccess.cc
// RsGxsGroupId is made from a sha1 hash which is reduced to 16 bytes later
//   this should be fixed to use full 20 sha1 bytes
//     but this is not backwards compatible because of fixed-length serialisation format
// make a interface for rsMsgs based on token system?
//   then not all msgs would have to stay in ram all time, but still have a not blocking load
//   we could store msgs in gxs, and sync them between locations
// what is RsItem.clear() good for?
//   is this ever used?
// gxs supports only event receiver
//   the multiplexing is done at the gui level
//   but what if there are different guis like one Qt and one Wt?

// about fb from b1
// profile page: visible for friends by default
// timeline: visible for friends, friends can post to it,
//           can post to own timeline
//           click "share" at content from others, to import into timeline
//              what i shared is public visible
// comments: can comment every root post
// like: have a personal list of likes, can like every content including comments
//       like on comments is juts for display at the comment
// fb page: one can create a page. this is a public place where you have the right to delete posts
//    others can share and like the page
//    very similar to a blog
//
// thoughts on rs: probably don't want to make a like public

// about likes
// store like in the liked gxs-group: have public likes
// store in own wall-grp: like can be private
//                        friends will know i liked something without being subscribed to the grp
//                        so freinds learn about new groups
// maybe combine both?
// always on own wall
// and if the user allows it make a public like
// public likes are good for the content creator, because he receives feedback
// he will then continue to make good content
// but can't prevent fake-likes
// likes in wall-grp would give a local but maybe higher quality view on the like situation

// with shared content: want to make reference at origin to the place where it got shared?
//   of course only if the place is public

// more about the local view
// a local view is something where i only see content from friends or friends + fofs
// idea: local view for rating files
//   can abuse the mOriginalMsgId to store file hash
//   then can request all comments for a file

// big question: how to remove/hide posts???
// own posts? posts from others?
// can block entire identities. is it possible to make a blacklist of posts with publish key?

// next steps
//  ui items for post and replies
//  make listmodel for newsfeed and wall (will have similar model with different parameters)
//  gen identity ui
//  list identity ui, maybe make this first, because backends is there

// things to do on first start:
//  create id
//  create circle for friends with author id set
//    how can we know if a circle is a friendslist or just a circle?
//    can't know this, so have to keep circle id somewhere
//    which name should the circle have?
//    maybe need some hacks in the circle system?
//      can abuse RsGroupMetaData.mServiceString to have a flag which shows "this is a friendslist"
//      better don't make hack.
//      better detect by circle group meta if this can be a friendslist
//      it can be a friendslist, if author is set. can be own friendslist if own author id set
//        on friends wall, filter circles by friends author id
//      what if A makes a circle named "friends of B", everyone will believe it is friends of B
//        display circles by athor id like: author -> circle name

// from which service can i steal items and serialiser?
// gxscommentitems look useful
// p3gxscommentservice has this complicated rating formula
// formula doesn not handle relations like friendship and follow
// how to request comments: GxsCommentTreeWidget::service_requestComments
// want to rate root posts only, display others ordered by time

// want avatars later, how to do this?
// maybe store it with the profile in a author-only group


// about rsitems:
// can't change these later, so design them carefully
// check if tlv items can be extended later
// but try to include all data we want before first release

// for the docs
// mechanisms in gxs
// define who can receive: circles
// verification:
//   publish sign
//   author sign


/*
WallItems

// root Posts
// group contains post itself an replys + votes
class WallPostGroup{
    RsGroupMetaData mMeta;
}
class WallPost{
    RsMsgMetaData mMeta;
    std::string mMsg;
}
class WallVote{
    RsMsgMetaData mMeta;
    uint8_t mVote;
}

// group contains grpIds of grps where we contributed to
class WallPointerGroup{
    RsGroupMetaData mMeta;
}
class WallPointer{
    RsMsgMetaData mMeta;
    uint8_t mType;

    const uint8_t WPTR_TYPE_COMMENT = 0;
    const uint8_t WPTR_TYPE_VOTE_UP = 1;    // want to know if up or downvote
    const uint8_t WPTR_TYPE_VOTE_DOWN = 2;  // don't want to subscibe to downvoted groups
}

class WallProfileGroup{
    RsGroupMetaData mMeta;
}
class WallProfileEntry{
    // want to have profile data splitted into different msgs?
    // this would allow cheap updating of single entrys
    //   this depends on size of msg meta
    //   msg meta has:
    //   4x msg id with 20 bytes
    //   1x grp id with 16 bytes
    //   1x author id with 16 bytes
    //   1x signatures, dont know how large    measured for gxs forum post 322 bytes
    //   probably is the textual profile data not so big, so put all text in one item
    //   pictures are big, so one pic per entry item
    //   item header is 2x4=8 byte

    // makes 120 + 322 + 8 = 450 bytes

    // more important: what should be on profile page?
    // look at other social networks
}

*/

class RsWallMsg{
public:
    RsMsgMetaData mMeta;
    std::string mMsg;
};

class RsWallGroup{
public:
    RsGroupMetaData mMeta;
};

/*
message and group changes propagation:

rsgenexchange calls
virtual void notifyChanges(std::vector<RsGxsNotify*>& changes) = 0;

have to implement this in own rsgenexchange derived class
then call
RsGxsIfaceHelper::receiveChanges(changes)

which then calls

void RsGenExchange::receiveChanges(std::vector<RsGxsNotify*>& changes)

why does this go through all layers and not directly?

rsgenexchange makes a list of this changes, and the it can be retrieved from gxsifacehelper,
whichs calls genexchange

next level is a class which polls gxsifacehelper, where widgets register to receive updates
gxsupdatebroadcast
*

/*
class RsWall: public RsGxsIfaceHelper{
public:
    virtual bool getGroupData(const uint32_t &token, std::vector<RsWallGroup> &groups) = 0;
    virtual bool getMsgData(const uint32_t &token, std::vector<RsWallMsg> &msgs) = 0;

    virtual void setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read) = 0;

    virtual bool createGroup(uint32_t &token, RsGxsForumGroup &group) = 0;
    virtual bool createMsg(uint32_t &token, RsGxsForumMsg &msg) = 0;

    // need to handle new msgs in groups
    // need to handle new groups, and subscribe if required
    // ui needs to receive new groups

    // ui displays news in a list
    // how to know size of list?
    // want to use dynamic loading of data from listmodel
    // maintain a list of group ids
    // need to sort groups? to make interestig posts first?
    // idea: have a list of all subscribed group ids, have to do this anyways
    // then rank the ids by:
    // last post
    // how many friends commented
    // likes of friends
};
*/
