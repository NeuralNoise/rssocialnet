#include "RSWappTestPage.h"

#include <Wt/WText>
#include <Wt/WContainerWidget>
#include <Wt/WImage>
#include <Wt/WStackedWidget>
#include <Wt/WMenu>
#include <Wt/WBorderLayout>
#include <Wt/WVBoxLayout>
#include <Wt/WDefaultLayout>
#include <Wt/WLineEdit>
#include <Wt/WPushButton>
#include <Wt/WBreak>

#include <Wt/WAbstractTableModel>
#include <Wt/WModelIndex>
#include <Wt/WItemDelegate>
#include <Wt/WTableView>

#include <Wt/WColor>

// todo: decouple from retroshare-gui
#include "../../retroshare-gui/src/util/TokenQueue.h"
#include <retroshare/rsidentity.h>

#include <Wt/WTimer>
// todo: check if gxstokenqueue(from libretroshare) should be used instead of TokenQueue which is a ui class
class TokenQueueWt: public Wt::WObject, public TokenQueueBase{
public:
    TokenQueueWt(RsTokenService *service, TokenResponse *resp):
        WObject(), TokenQueueBase(service, resp), mTimer(){
        mTimer.timeout().connect(this, &TokenQueueWt::onTimer);
        mTimer.setSingleShot(true);
    }
    void onTimer(){
        pollRequests();
    }

protected:
    virtual void doPoll(float dt){
        mTimer.setInterval(dt*1000);
        mTimer.start();
    }
private:
    Wt::WTimer mTimer;
};

const uint32_t IDENTITY_MODEL_TOKENTYPE_LIST_IDS = 0;

class IdentityModel: public Wt::WAbstractTableModel, public TokenResponse{
public:
    IdentityModel(): WAbstractTableModel(), mTokenQueue(rsIdentity->getTokenService(), this)
    {
        update();
    }

    void update()
    {
        mTokenQueue.cancelActiveRequestTokens(IDENTITY_MODEL_TOKENTYPE_LIST_IDS);

        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

        uint32_t token;

        mTokenQueue.requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, IDENTITY_MODEL_TOKENTYPE_LIST_IDS);
    }
    virtual void loadRequest(const TokenQueueBase *queue, const TokenRequest &req)
    {
        std::cerr << "IdentityModel::loadRequest() UserType: " << req.mUserType << std::endl;

        switch(req.mUserType)
        {
            case IDENTITY_MODEL_TOKENTYPE_LIST_IDS:
                // tell views about data change in model
                reset();
                mIds.clear();
                rsIdentity->getGroupData(req.mToken, mIds);
                break;
            default:
                std::cerr << "IdentityModel::loadRequest() ERROR unknown UserType";
                std::cerr << std::endl;
                break;
        }
    }

    int rowCount(const Wt::WModelIndex &parent=Wt::WModelIndex()) const
    {
        if(!parent.isValid()){
            return mIds.size();
        }else{
            return 0;
        }
    }

    int columnCount(const Wt::WModelIndex &parent=Wt::WModelIndex()) const
    {
        if(!parent.isValid()){
            return 3;
        }else{
            return 0;
        }
    }

    boost::any data(const Wt::WModelIndex &index, int role) const
    {
        const RsGxsIdGroup &id = mIds[index.row()];
        switch(role){
        case Wt::DisplayRole:
            switch(index.column()){
            case 0:
                return Wt::WString::fromUTF8(id.mMeta.mGroupName);
            case 1:
                return Wt::WString::fromUTF8(id.mMeta.mGroupId.toStdString());
            case 2:
                return Wt::WString::fromUTF8(id.mPgpId.toStdString());
            }
        default:
            return boost::any();
        }
    }
private:
    std::vector<RsGxsIdGroup> mIds;
    TokenQueueWt mTokenQueue;
};

class FeedDelegate: public Wt::WItemDelegate{
public:
    virtual Wt::WWidget* update(Wt::WWidget *widget, const Wt::WModelIndex &index, Wt::WFlags<Wt::ViewItemRenderFlag> /*flags*/)
    {
        Wt::WText *text = dynamic_cast<Wt::WText*>(widget);
        if(!text){
            text = new Wt::WText();
        }
        text->setText(Wt::WString::fromUTF8(boost::any_cast<std::string>(index.data(Wt::UserRole)).c_str()));
        return text;
    }
};

class FeedModel: public Wt::WAbstractTableModel{
public:
    FeedModel(): WAbstractTableModel()
    {

    }

    int rowCount(const Wt::WModelIndex &parent=Wt::WModelIndex()) const
    {
        if(!parent.isValid()){
            return mContent.size();
        }else{
            return 0;
        }
    }

    int columnCount(const Wt::WModelIndex &parent=Wt::WModelIndex()) const
    {
        if(!parent.isValid()){
            return 1;
        }else{
            return 0;
        }
    }

    boost::any data(const Wt::WModelIndex &index, int role) const
    {
        switch(role){
        case Wt::UserRole:
            return mContent[index.row()];
        default:
            return boost::any();
        }
    }

    // call this before using this model
    // todo: handle addition and removal of items when in use
    void addSomething(std::string something)
    {
        mContent.push_back(something);
    }
private:
    std::vector<std::string> mContent;
};

class FeedWidget: public Wt::WContainerWidget{
public:
    FeedWidget(): WContainerWidget()
    {
        Wt::WBorderLayout *layout = new Wt::WBorderLayout();
        this->setLayout(layout);

        Wt::WImage *avatar = new Wt::WImage();
        avatar->resize(80, 80);
        // load image from memory
        //Wt::WMemoryResource();
        Wt::WText *text = new Wt::WText(Wt::WString("hi there"));

        // a BorderLayout can only handle one widget in each region
        // else will crash
        // docs also say only one widget
        // so make another container and place widgets there
        mCenterContainer = new Wt::WContainerWidget();

        mCenterContainer->addWidget(text);
        // put avatarin own container, to prevent stretching from BorderLayout
        Wt::WContainerWidget *avatarContainer = new Wt::WContainerWidget();
        avatarContainer->addWidget(avatar);
        layout->addWidget(avatarContainer, Wt::WBorderLayout::West);
        layout->addWidget(mCenterContainer, Wt::WBorderLayout::Center);

    }
    void postWidget_addReply(WWidget *child){
        mCenterContainer->addWidget(child);
    }

private:
    Wt::WContainerWidget *mCenterContainer;
};

RSWappTestPage::RSWappTestPage(Wt::WContainerWidget *parent):
    WCompositeWidget(parent)
{
    tokenQueue = new TokenQueueWt(rsIdentity->getTokenService(), this);
    // allow automatic destruction
    Wt::WObject::addChild(tokenQueue);

    setImplementation(_impl = new Wt::WContainerWidget());

    Wt::WBorderLayout *layout = new Wt::WBorderLayout();
    _impl->setLayout(layout);

    // Create a stack where the contents will be located.
    Wt::WStackedWidget *contents = new Wt::WStackedWidget();

    Wt::WMenu *menu = new Wt::WMenu(contents, Wt::Vertical);
    layout->addWidget(menu, Wt::WBorderLayout::West);
    layout->addWidget(contents, Wt::WBorderLayout::Center);
    //menu->setStyleClass("nav nav-pills nav-stacked");
    menu->setWidth(150);

    FeedWidget *w1 = new FeedWidget();
    FeedWidget *w2 = new FeedWidget();
    FeedWidget *w3 = new FeedWidget();
    w1->postWidget_addReply(w2);
    w1->postWidget_addReply(w3);

    Wt::WTableView *tableView = new Wt::WTableView();
    FeedModel *model = new FeedModel();
    model->addSomething(std::string("eins"));
    model->addSomething(std::string("zwei<br/>zwei punkt zwei"));
    model->addSomething(std::string("drei"));
    // this does not work
    // table has fixed 20px height, not good
    // table renders all rows with same height, not good
    // other idea: just add button to load more posts
    //tableView->setRowHeight(Wt::WLength::Auto);
    tableView->setModel(model);
    tableView->setItemDelegate(new FeedDelegate());
    tableView->setHeaderHeight(0);
    tableView->setColumnBorder(Wt::WColor(0,0,0,0));

    // IdentityModel test
    Wt::WTableView *tableViewIdentities = new Wt::WTableView();
    IdentityModel *identityModel = new IdentityModel();
    idModel = identityModel;
    tableViewIdentities->setModel(identityModel);

    Wt::WPushButton *buttonNewId = new Wt::WPushButton("create new identity");
    buttonNewId->clicked().connect(this, &RSWappTestPage::showNewIdDialog);

    Wt::WContainerWidget *idContainer = new Wt::WContainerWidget();
    idContainer->addWidget(buttonNewId);
    idContainer->addWidget(tableViewIdentities);

    // Add menu items using the default lazy loading policy.
    menu->addItem("feed", w1);
    menu->addItem("tableView", tableView);
    menu->addItem("eins", new Wt::WText("eins"));
    menu->addItem("zwei", new Wt::WText("zwei"));
    menu->addItem("IdentityView", idContainer);
}

void RSWappTestPage::showNewIdDialog()
{
    newIdDialog = new Wt::WDialog("create a new Identity");
    new Wt::WText("name", newIdDialog->contents());
    Wt::WLineEdit *edit = new Wt::WLineEdit(newIdDialog->contents());
    newIdDialogLineEdit = edit;
    new Wt::WBreak(newIdDialog->contents());

    Wt::WPushButton *ok = new Wt::WPushButton("Ok", newIdDialog->contents());
    Wt::WPushButton *cancel = new Wt::WPushButton("cancel", newIdDialog->contents());
    // these events will accept() the Dialog
    edit->enterPressed().connect(newIdDialog, &Wt::WDialog::accept);
    ok->clicked().connect(newIdDialog, &Wt::WDialog::accept);
    cancel->clicked().connect(newIdDialog, &Wt::WDialog::reject);

    newIdDialog->finished().connect(this, &RSWappTestPage::newIdDialogDone);
    newIdDialog->show();
}

void RSWappTestPage::newIdDialogDone(Wt::WDialog::DialogCode code)
{
    if (code == Wt::WDialog::Accepted){
        std::cerr << "create new id clicked ok" << std::endl;
        RsIdentityParameters params;
        params.nickname = newIdDialogLineEdit->text().toUTF8();
        // todo: pgp password callback
        params.isPgpLinked = false;

        uint32_t token = 0;
        rsIdentity->createIdentity(token, params);
        // queue callback when id was created, to updates views then
        tokenQueue->queueRequest(token, 0, 0, 0);

    }
    delete newIdDialog;
}

void RSWappTestPage::loadRequest(const TokenQueueBase *queue, const TokenRequest &req){
    idModel->update();
}
