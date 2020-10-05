#include "database.h"

#include <ctime>

// #define DB_LOGGING

// ReSharper disable CppInconsistentNaming
const char* TABLE_BOTS = "bots";
const char* COL_B_NAME = "username";
const char* COL_B_CLAN = "clan";

const char* CREATE_TABLE_BOTS = \
"CREATE TABLE bots ("
"    username TEXT NOT NULL,"
"    clan TEXT NOT NULL,"
"    CONSTRAINT username_clan_pair_unique UNIQUE ("
"        username,"
"        clan"
"    ),"
"    PRIMARY KEY ("
"        username,"
"        clan"
"    )"
");";

const char* TABLE_DETECTIONS = "detections";
const char* COL_D_NAME = "username";
const char* COL_D_CLAN = "clan";
const char* COL_D_IP = "ip";
const char* COL_D_VERSION = "version";
const char* COL_D_FLAGS = "flags";
const char* COL_D_SERVER = "servername";
const char* COL_D_GAMEMODE = "gamemode";
const char* COL_D_VS_BOTS = "vs_bots";
const char* COL_D_TIMESTAMP = "timestamp";

const char* CREATE_TABLE_DETECTIONS = \
"CREATE TABLE detections ("
"    id         INTEGER PRIMARY KEY AUTOINCREMENT"
"                       NOT NULL,"
"    username   TEXT    NOT NULL,"
"    clan       TEXT    NOT NULL,"
"    version    INTEGER NOT NULL,"
"    flags      INTEGER NOT NULL,"
"    ip         TEXT    NOT NULL,"
"    servername TEXT    NOT NULL,"
"    gamemode   TEXT    NOT NULL,"
"    vs_bots    BOOLEAN NOT NULL,"
"    timestamp  INTEGER NOT NULL,"
"    FOREIGN KEY ("
"        username,"
"        clan"
"    )"
"    REFERENCES bots (username,"
"    clan) "
");";
// ReSharper restore CppInconsistentNaming

int execute_and_print(sqlite3* db, const char* statement, const sqlite3_callback x_callback, void* data, const char* message)
{
	char* message_error;
	#ifdef DB_LOGGING
	std::cerr << "Statement: " << statement << std::endl;
	#endif
	const auto res = sqlite3_exec(db, statement, x_callback, data, &message_error);
	if (res != SQLITE_OK) {
		#ifdef DB_LOGGING
		std::cerr << message << ": Failed (" << message_error << ")" << std::endl;
		#endif
		sqlite3_free(message_error);
	}
	else
	{
		#ifdef DB_LOGGING
		std::cout << message << ": OK" << std::endl;
		#endif
	}
	return res;
}

static int user_version_callback(void* data, const int argc, char** argv, char** az_col_name)
{
	if (argc > 0) {
		static_cast<CDatabase*>(data)->user_version = atoi(argv[0]);
	}

	return 0;
}

#include <random>

int CDatabase::Open(const std::string& path)
{
	if (m_db != nullptr) {
		sqlite3_close(m_db);
	}
	auto res = sqlite3_open(path.c_str(), &m_db);

	if (res) {
		#ifdef DB_LOGGING
		std::cerr << "Error open DB " << sqlite3_errmsg(m_db) << std::endl;
		#endif
		return res;
	}

	execute_and_print(m_db, "PRAGMA user_version;", user_version_callback, this, "Get User Version");
	res = execute_and_print(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, "Set PRAGMA foreign_keys = ON");

	if (user_version == 0)
	{
		sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

		res += execute_and_print(m_db, CREATE_TABLE_BOTS, nullptr, nullptr, "Create table bots");
		res += execute_and_print(m_db, CREATE_TABLE_DETECTIONS, nullptr, nullptr, "Create table detections");

		if (res) {
			#ifdef DB_LOGGING
			std::cerr << "Error open DB " << sqlite3_errmsg(m_db) << std::endl;
			#endif
			return res;
		}
		execute_and_print(m_db, "PRAGMA user_version = 2;", user_version_callback, this, "Set User Version to 2");

		res = sqlite3_exec(m_db, "END TRANSACTION;", nullptr, nullptr, nullptr);

		if (res != SQLITE_OK) {
			user_version = 2;
		}
	}

	if (user_version == 1)
	{
		sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

		res += execute_and_print(m_db, CREATE_TABLE_DETECTIONS, nullptr, nullptr, "Create table detections");

		if (res) {
			#ifdef DB_LOGGING
			std::cerr << "Error open DB " << sqlite3_errmsg(m_db) << std::endl;
			#endif
			return res;
		}
		execute_and_print(m_db, "PRAGMA user_version = 2;", user_version_callback, this, "Set User Version to 2");

		res = sqlite3_exec(m_db, "END TRANSACTION;", nullptr, nullptr, nullptr);

		if (res != SQLITE_OK) {
			user_version = 2;
		}
	}

	return res;
}

bool CDatabase::IsBot(const std::string& username, const std::string& clan)
{
	sqlite3_stmt* stmt = nullptr;
	auto res = sqlite3_prepare_v2(
		m_db,
		sqlite3_mprintf("SELECT * FROM %s where %s == %Q and %s == %Q;", TABLE_BOTS, COL_B_NAME, username.c_str(), COL_B_CLAN, clan.c_str()),
		-1,
		&stmt,
		nullptr);

	if (res) {
		#ifdef DB_LOGGING
		std::cerr << "Error open DB " << sqlite3_errmsg(m_db) << std::endl;
			#endif
		return false;
	}

	for (;;) {
		res = sqlite3_step(stmt);

		if (res == SQLITE_ROW) {
			return true;
		}

		if (res == SQLITE_DONE) {
			return false;
		}

		if (res) {
			#ifdef DB_LOGGING
			std::cerr << "Error open DB " << sqlite3_errmsg(m_db) << std::endl;
			#endif
			return false;
		} else {
			return false;
		}
	}

	return false;
}

bool CDatabase::DetectionTracked(
	const std::string& username,
	const std::string& clan,
	const std::string& ip,
	const std::string& servername,
	const std::string& gamemode,
	const int version,
	const int flags,
	const bool vs_bots)
{
	sqlite3_stmt* stmt = nullptr;
	auto res = sqlite3_prepare_v2(
		m_db,
		sqlite3_mprintf("SELECT * FROM %s where %s == %Q and %s == %Q and %s == %Q and %s == %Q and %s == %Q and %s == %d and %s == %d and %s == %d;",
			TABLE_DETECTIONS,
			COL_D_NAME,
			username.c_str(),
			COL_D_CLAN,
			clan.c_str(),
			COL_D_IP,
			ip.c_str(),
			COL_D_SERVER,
			servername.c_str(),
			COL_D_GAMEMODE,
			gamemode.c_str(),
			COL_D_VERSION,
			version,
			COL_D_FLAGS,
			flags,
			COL_D_VS_BOTS,
			vs_bots
		),
		-1,
		&stmt,
		nullptr);

	if (res) {
		#ifdef DB_LOGGING
		std::cerr << "Error open DB " << sqlite3_errmsg(m_db) << std::endl;
			#endif
		return false;
	}

	for (;;) {
		res = sqlite3_step(stmt);

		if (res == SQLITE_ROW) {
			return true;
		}

		if (res == SQLITE_DONE) {
			return false;
		}

		if (res) {
			#ifdef DB_LOGGING
			std::cerr << "Error open DB " << sqlite3_errmsg(m_db) << std::endl;
			#endif
			return false;
		} else {
			return false;
		}
	}

	return false;
}

void CDatabase::AddBot(const std::string& username, const std::string& clan, const std::string& ip, const std::string& servername, const std::string& gamemode, const int version, const int flags, const bool vs_bots)
{
	if (!IsBot(username, clan)) {
		auto res = execute_and_print(
			m_db,
			sqlite3_mprintf("INSERT INTO %s(%s, %s) VALUES(%Q, %Q);", TABLE_BOTS, COL_B_NAME, COL_B_CLAN, username.c_str(), clan.c_str()),
			nullptr,
			nullptr,
			"insert bot"
		);

		if (res) {
			#ifdef DB_LOGGING
			std::cerr << "Error open DB " << sqlite3_errmsg(m_db) << std::endl;
			#endif
			return;
		}
	}
	if (!DetectionTracked(username, clan, ip, servername, gamemode, version, flags, vs_bots)) {
		auto res = execute_and_print(
			m_db,
			sqlite3_mprintf(
				"INSERT INTO %s(%s, %s, %s, %s, %s, %s, %s, %s, %s) VALUES(%Q, %Q, %Q, %Q, %Q, %d, %d, %d, %d);",
				TABLE_DETECTIONS,
				COL_D_NAME,
				COL_D_CLAN,
				COL_D_IP,
				COL_D_SERVER,
				COL_D_GAMEMODE,
				COL_D_VERSION,
				COL_D_FLAGS,
				COL_D_VS_BOTS,
				COL_D_TIMESTAMP,
				username.c_str(),
				clan.c_str(),
				ip.c_str(),
				servername.c_str(),
				gamemode.c_str(),
				version,
				flags,
				vs_bots,
				std::time(0)
			),
			nullptr,
			nullptr,
			"insert ip"
		) == SQLITE_OK;

		if (res) {
			#ifdef DB_LOGGING
			std::cerr << "Error open DB " << sqlite3_errmsg(m_db) << std::endl;
			#endif
			return;
		}
	}
}