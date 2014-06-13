#include "WallRootPostWidget.h"

#include <Wt/WBorderLayout>
#include <Wt/WHBoxLayout>

#include "rswall.h"
#include "RsGxsUpdateBroadcastWt.h"


WallRootPostWidget::WallRootPostWidget(RsGxsGroupId grpId, Wt::WContainerWidget *parent):
    WContainerWidget(parent), _mTokenQueue(rsWall->getTokenService()),_mGrpId(grpId)
{
    Wt::WBorderLayout *layout = new Wt::WBorderLayout();
    //Wt::WHBoxLayout *layout = new Wt::WHBoxLayout();
    this->setLayout(layout);

    _mAvatarWidget = new AvatarWidget(false);
    layout->addWidget(_mAvatarWidget, Wt::WBorderLayout::West);
    //layout->addWidget(_mAvatarWidget);


    // a BorderLayout can only handle one widget in each region
    // else will crash
    // docs also say only one widget
    // so make another container and place widgets there
    _mCenterContainer = new Wt::WContainerWidget();
    layout->addWidget(_mCenterContainer, Wt::WBorderLayout::Center);
    //layout->addWidget(_mCenterContainer);

    _mText = new Wt::WLabel(_mCenterContainer);

    _CommentContainer = new CommentContainerWidget(rsWall, rsWall, _mCenterContainer);

    RsGxsUpdateBroadcastWt::get(rsWall)->grpsChanged().connect(this, &WallRootPostWidget::grpsChanged);

    _mTokenQueue.tokenReady().connect(this, &WallRootPostWidget::onTokenReady);

    setGroupId(grpId);
}

void WallRootPostWidget::setGroupId(RsGxsGroupId &grpId)
{
    _mGrpId = grpId;
    // request stuff
    uint32_t token;
    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
    std::list<RsGxsGroupId> grpIds;
    grpIds.push_back(grpId);
    rsWall->getTokenService()->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, grpIds);

    _mTokenQueue.queueToken(token);

    _mText->setText("PENDING");
}

void WallRootPostWidget::onTokenReady(uint32_t token, bool ok)
{
    if(ok)
    {
        PostMsg pm;
        rsWall->getPostMsg(token, pm);
        _mAvatarWidget->setIdentity(pm.mMeta.mAuthorId);

        _mText->setText(Wt::WString::fromUTF8(pm.mPostText));

        _CommentContainer->setGrpMsgId(RsGxsGrpMsgIdPair(pm.mMeta.mGroupId, pm.mMeta.mMsgId));
    }
    else
    {
        _mText->setText("FAIL");
    }
}

void WallRootPostWidget::grpsChanged(const std::list<RsGxsGroupId> &grps)
{
    //
}
