#include "../Interface/Service.h"
#include "../Interface/CustomClient.h"
#include "../Interface/Server.h"
#include "../urbackupcommon/fileclient/tcpstack.h"
#include "../urbackupcommon/internet_pipe_capabilities.h"
#include "server_settings.h"
#include <queue>

class IMutex;
class ICondition;
class InternetServicePipe;
class CompressedPipe;

class InternetService : public IService
{
	virtual ICustomClient* createClient();
	virtual void destroyClient( ICustomClient * pClient);
};

enum InternetServiceState
{
	ISS_AUTH,
	ISS_AUTHED,
	ISS_CAPA,
	ISS_CONNECTING,
	ISS_USED,
	ISS_QUIT
};


class InternetServiceConnector;

struct SClientData
{
	std::vector<InternetServiceConnector*> spare_connections;
	unsigned int last_seen;
};

struct SOnetimeToken
{
	SOnetimeToken(const std::string &clientname)
		: clientname(clientname)
	{
		created=Server->getTimeMS();
		token=ServerSettings::generateRandomBinaryKey();
	}
	std::string token;
	unsigned int created;
	std::string clientname;
};

const char SERVICE_COMMANDS=0;
const char SERVICE_FILESRV=1;

class InternetServiceConnector : public ICustomClient
{
public:
	InternetServiceConnector(void);
	~InternetServiceConnector(void);
	virtual void Init(THREAD_ID pTID, IPipe *pPipe);

	virtual bool Run(void);
	virtual void ReceivePackets(void);

	static void init_mutex(void);
	static void destroy_mutex(void);

	static IPipe *getConnection(const std::string &clientname, char service, int timeoutms=-1);
	static std::vector<std::string> getOnlineClients(void);

	void connectStart();
	void Connect(ICondition *n_cond, ICondition *stop_cond, char service);
	void stopConnecting(void);
	void stopConnectingAndWait(void);
	void freeConnection(void);

	virtual bool wantReceive(void);
	virtual bool closeSocket(void);

	IPipe *getISPipe(void);
	
	bool waitForConnection(ICondition *cond, int timems);
private:
	void operator=(const InternetServiceConnector& other){}
	void operator()(const InternetServiceConnector& other){}
	InternetServiceConnector(const InternetServiceConnector& other){}

	void cleanup_pipes(bool remove_connection);
	void do_stop_connecting(void);

	std::string  generateOnetimeToken(const std::string &clientname);
	std::string getOnetimeToken(unsigned int id, std::string *cname);
	static void removeOldTokens(void);

	std::string getAuthkeyFromDB(const std::string &clientname, bool &db_timeout);

	static std::map<std::string, SClientData> client_data;
	static IMutex *mutex;

	int state;

	THREAD_ID tid;
	IPipe *cs;
	InternetServicePipe *is_pipe;
	CompressedPipe *comp_pipe;
	IPipe *comm_pipe;
	IMutex *local_mutex;
	ICondition* connection_done_cond;
	ICondition* connection_stop_cond;

	CTCPStack tcpstack;

	unsigned int starttime;
	unsigned int lastpingtime;
	bool pinging;
	bool has_timeout;

	bool connect_start;
	volatile bool do_connect;
	volatile bool stop_connecting;
	bool is_connected;
	volatile bool free_connection;

	char target_service;

	std::string clientname;
	std::string challenge;
	std::string authkey;

	int compression_level;

	bool token_auth;

	static IMutex *onetime_token_mutex;
	static std::map<unsigned int, SOnetimeToken> onetime_tokens;
	static unsigned int onetime_token_id;

	static unsigned int last_token_remove;
};
