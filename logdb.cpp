//
// Created by krazh on 03.11.17.
//

#include "logdb.h"
#include "logdb_c_cpp.h"

#include "EncodeConvertor.h"
#include "termcolor.hpp"


using namespace std;


      uint64_t loggerDBSize      = 0;       // ���� ��� �࠭���� ࠧ��� ��(���-�� ����ᥩ)
      uint64_t timeLoggerDBSize  = 0;       // ���� ��� �࠭���� ࠧ��� ��(���-�� ����ᥩ)
      uint64_t fdLoggerDBSize    = 0;         // ���� ��� �࠭���� ࠧ��� ��(���-�� ����ᥩ)
      uint64_t pulseDBSize       = 0;         // ���� ��� �࠭���� ࠧ��� ��(���-�� ����ᥩ)
      uint8_t  countLoggerTables = 0;       // ���� ��� �࠭���� ������⢠ ⠡���
const uint8_t  NUM_OF_TABLES     = 4;       // ��饥 �᫮ ⠡��� ������

mutex mutexLogDB;                   // ��������� ��⥪� ���饭�� � �� ����


#define MAX_MESS_SIZE  65536

char fmt_new[MAX_MESS_SIZE],
        mess[MAX_MESS_SIZE];     // ���� ��� ᮮ�饭��


//===================================================================================
// ��������� � ��䮫�멬� ���祭�ﬨ
Log_DB::Log_DB()
{
    _work.store(true);
    dB_Name = "./logDb.db";     // ��� �� ����                   Android path: /data/data/org.qtproject.example.testAppOldUi/files
    maxQuerySize = 1000;         // ���� �᫮ ᮮ�饭��, �������� ������ � �� ����

    maxDBSize = 20000;          // ���� ������⢮ ����ᥩ � �� ����

    writeDBPeriod = 500000;        // ��ਮ� ����� � �� ���� ᮮ�饭�� � ����ᥪ㭤��

    logLevel = LOG_LEVELS::WARNING_L;

    // ���⪠ ��।� ᮮ�饭�� ��� ��⥪ᮬ
    mutexQuery.lock();
    messagesQuery.clear();
    mutexQuery.unlock();
}


//===================================================================================
// ��������
Log_DB::~Log_DB()
{
    // ���⪠ ��।� ᮮ�饭�� ��� ��⥪ᮬ
    mutexQuery.lock();
    messagesQuery.clear();
    mutexQuery.unlock();
}

//===================================================================================
// log_ERR
bool Log_DB::log_ERR (const LOG_REGIONS region, string  strMess )
{
    //cout << "_2_ >>>>>>>>>>>>> log_ERR :: START! strMess = |" << fmt << "|" << endl;
    return _log_in_sql(LOG_LEVELS::ERROR_L, region, strMess);
}


//===================================================================================
// log_WARN
bool Log_DB::log_WARN (const LOG_REGIONS region, string  strMess )
{
    //cout << "_2_ >>>>>>>>>>>>> log_WARN :: START! strMess = |" << strMess << "|" << endl;
    return _log_in_sql(LOG_LEVELS::WARNING_L, region, strMess);

}


//===================================================================================
// log_INFO
bool Log_DB::log_INFO (const LOG_REGIONS region,  string  strMess )
{
    //cout << "_2_ >>>>>>>>>>>>> log_INFO :: START! strMess = |" << strMess << "|" << endl;
    return _log_in_sql(LOG_LEVELS::INFO_L, region, strMess);
}


//===================================================================================
// log_DBG
bool Log_DB::log_DBG (const LOG_REGIONS region,  string  strMess )
{
    //cout << "_2_ >>>>>>>>>>>>> log_DBG :: START! strMess = |" << fmt << "|" << endl;
    return _log_in_sql(LOG_LEVELS::DEBUG_L, region, strMess);
}

#include <QDir>
//===================================================================================
// ��⮤, ����� ������ ���� ���饭 � ��. ��⮪�, �㤥� ����� ᮮ�饭�� � �� �� ��।�(�����) ᮮ�饭��. ��横���.
void Log_DB::logDaemon()
{
    int qSize = 0;
    qDebug() << "logDaemon():: START" << endl;
    QDir dir;
    _work.store(true);
    while(_work.load())
    {
        qSize = messagesQuery.size();
        // �᫨ ���� ᮮ�饭�� �� ������ � ��
        if( qSize > 0 )
        {
            // �஢��塞 ���� �� �� ����
            if(!_isLogDBExist())
            {
                qDebug() << "logDaemon()::  _createLoggerTable()..." << endl;
                // ��⠥��� ᮧ���� �� ����
                if( !_createLoggerTable() )
                {
                    qDebug() << "logDaemon()::  Error! Can't create table in log DB. Start regeneration!" << endl;
                    string strRm = "rm -r " + dB_Name;
                    qDebug() << "logDaemon():: regeneration! rm -r=" << strRm.c_str() << endl;
                    remove(strRm.c_str());
                    qDebug() << "logDaemon():: regeneration! sync..." << endl;
                    sync();
                    qDebug() << "logDaemon():: regeneration! sleep..." << endl;
                    sleep(2);
                    qDebug() << "logDaemon():: regeneration! rm -r continue!" << endl;

                    continue;
                }
                else
                {
                    qDebug() << "logDaemon()::  DATABASE CREATED! name = '" << logger.getDBName().c_str() << "'" << endl;
                }
            }
            // ������ � �� ���� ������ ᮮ�饭��
            if( _writeMessQToDB())
            {
                // ������塞 ���� � ࠧ��� ��
                _sizeOfLogDB();
                if( (loggerDBSize > maxDBSize) || (timeLoggerDBSize > maxDBSize) || (fdLoggerDBSize > maxDBSize) || (pulseDBSize > maxDBSize) )
                {
                    if( !_deleteFromLogDB ( (int)(maxDBSize / 2) ) )
                    {
                        _log_in_sql(LOG_LEVELS ::ERROR_L, LOG_REGIONS::REG_DATABASE, "CAN'T DELETE RECORDS!");
                    }
                }
            }
            else // �訡�� �� �����
            {
                qDebug() << "logDaemon():: Error! Can't write mess'" << messagesQuery.front().mess.c_str() << "' in log DB" << endl;
            }

        }// if
        else
        {
//            qDebug() << " logDaemon():: No Messages" << endl;
        }
        this_thread::sleep_for(chrono::microseconds(writeDBPeriod));
    }// while
}




//===================================================================================
//===================================================================================
//=============  PRIVATE ������               =======================================
//===================================================================================
//===================================================================================

//===================================================================================
//callbackLogger - �㭪�� ��ࠡ�⪨ �⢥� �� �� �� sql �����

static int _callbackLogger(void *data, int argc, char **argv, char **azColName)
{
//    qDebug() << " callbackLogger:: Start" << endl;
    string          strLoggerSize;          // ��ப� � ����ᠭ�� ࠧ��஬ ��

    for(int i = 0; i < argc; i++)
    {
        string column = _charToString((char *)(azColName[i]));

        // �᫨ ����訢��� �᫮ ����ᥩ
        if(column.compare("COUNT(*)") == 0)
        {
            strLoggerSize = _charToString((char *)(argv[i] ? argv[i] : "NULL"));
            if(strLoggerSize.compare("NULL") == 0)
            {
                loggerDBSize = 0;
                qDebug() << " _callbackLogger():: loggerDBSize = NULL" << endl;
            }
            else
            {
                loggerDBSize = stoul(strLoggerSize.c_str());
            }

        }
        // �᫨ ����訢��� �᫮ ����ᥩ
        if(column.compare("count(name)") == 0)
        {
            strLoggerSize = _charToString((char *)(argv[i] ? argv[i] : "NULL"));
            if(strLoggerSize.compare("NULL") == 0)
            {
                countLoggerTables = 0;
                qDebug() << " _callbackLogger():: countLoggerTables = NULL" << endl;
            }
            else
            {
                countLoggerTables = stoi(strLoggerSize.c_str());
            }

        }

        // �᫨ ���ᨬ �����
        string value = _charToString((char *)(argv[i] ? argv[i] : "NULL"));
    }
    return 0;
}


//===================================================================================
// �믮������ ����ᮢ � �� (����⨥+�����+����祭�� �⢥�)
bool Log_DB::_makeLoggerRequest()
{
    int rc = 0;
    char *zErrMsg = 0;

    if (dB_Name.empty())
    {
        qDebug() << " _makeLoggerRequest():: Error! Database name is empty!" << endl;
    }
    //open loggerDB
    rc = sqlite3_open( dB_Name.c_str(), &loggerDb);

    if( rc )
    {
        qDebug() << " _makeLoggerRequest():: Error! CAN'T OPEN '"<< dB_Name.c_str()
             << "' ERR MESS = "              <<  sqlite3_errmsg(loggerDb)
             << endl;
        return false;
    }
    else
    {
        //cout << "sqlRequest.c_str() = " << sqlRequest.c_str() << endl;
    }
    // �믮������ ����� � ��
    rc = sqlite3_exec(loggerDb, sqlRequest.c_str(), _callbackLogger, (void*)this, &zErrMsg);
    if( rc != SQLITE_OK )
    {
        qDebug() << " _makeLoggerRequest():: Error! SQL ERROR: " << endl
             << zErrMsg                  << endl
             << sqlite3_errmsg(loggerDb) << endl
             << rc                       << endl ;

        sqlite3_free(zErrMsg) ;
        return false;
    }
    sqlite3_close(loggerDb);
    if(zErrMsg != nullptr)
    {
        sqlite3_free(zErrMsg);
    }
    sync();
    return true;
}


//===================================================================================
// ��⮤ ᮧ����� ⠡���� ������
bool Log_DB::_createLoggerTable()
{
    lock_guard<mutex> locker(mutexLogDB);
    sqlRequest = "CREATE TABLE IF NOT EXISTS LOG("
            "ID        INTEGER PRIMARY KEY AUTOINCREMENT,"  // ���� ID(���稪)
            "DT        datetime default current_timestamp," // ��������� ᮮ�饭��
            "MESS          TEXT,"                           // ⥫� ᮮ�饭��
            "LVL        INTEGER,"                           // �஢���(���, �訡��...)
            "REGION     INTEGER);";                         // �������(��, ��ᯫ�� � �)

    sqlRequest+= "CREATE TABLE IF NOT EXISTS TIME_LOG("
                 "ID        INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "DT        datetime default current_timestamp," // ��������� ᮮ�饭��
                 "MESS          TEXT);";                           // ⥫� ᮮ�饭��

    sqlRequest+= "CREATE TABLE IF NOT EXISTS FD_LOG("
                 "ID        INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "DT        datetime default current_timestamp," // ��������� ᮮ�饭��
                 "MESS           TEXT);";                           // ⥫� ᮮ�饭��

    sqlRequest+= "CREATE TABLE IF NOT EXISTS PULSE_LOG("
                 "ID        INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "DT        datetime default current_timestamp," // ��������� ᮮ�饭��
                 "MESS           TEXT);";                           // ⥫� ᮮ�饭��

    sqlRequest+= "CREATE TRIGGER IF NOT EXISTS COPY_TIME_EVENTS AFTER INSERT ON LOG "
                 "WHEN NEW.REGION= "+ to_string(REG_TIME) +
                 " BEGIN "
                 "INSERT INTO TIME_LOG(MESS) VALUES (NEW.MESS); "
                 "END;";

    sqlRequest+= "CREATE TRIGGER IF NOT EXISTS COPY_FD_EVENTS AFTER INSERT ON LOG "
                 "WHEN NEW.REGION= "+ to_string(REG_FD) +
                 " BEGIN "
                 "INSERT INTO FD_LOG(MESS) VALUES (NEW.MESS); "
                 "END;";

    sqlRequest+= "CREATE TRIGGER IF NOT EXISTS COPY_PULSE_EVENTS AFTER INSERT ON LOG "
                 "WHEN NEW.REGION= "+ to_string(REG_PULSE) +
                 " BEGIN "
                 "INSERT INTO PULSE_LOG(MESS) VALUES (NEW.MESS); "
                 "END;";

    // �믮������ �����
    if( !_makeLoggerRequest() )
    {
        qDebug() << "_createLoggerTable()::FAILED TO CREATE LOG TABLES!" << endl;
        return false;
    }
    sync();
    // -----------------------------------
    return true;
}


//=================================================
//��⮤ ���������� ᮮ�饭�� � �� ����
bool Log_DB::_writeMessQToDB( )
{
    // �᫨ ��� ��।� ᮮ�饭�� - ���� ��室��
    if(messagesQuery.empty())
    {
        return true;
    }
    // -----------------------------------
    mutexLogDB.lock();
    mutexQuery.lock();
    sqlRequest.clear();
    // ��ନ஢���� SQL ����� �� ��।� ᮮ�饭��
    sqlRequest = "INSERT INTO LOG(MESS,LVL,REGION) VALUES(";
    for( unsigned int i =0; i < messagesQuery.size(); i++)
    {
        sqlRequest +=   "'" + messagesQuery.at(i).mess                      + "',"
                        + to_string(messagesQuery.at(i).levelOfMess)        + ","
                        + to_string(messagesQuery.at(i).regionOfMess);
        // �����襭�� �����
        sqlRequest +=   ((i+1) < messagesQuery.size())
                        ? "),("
                        : ");" ;
    }
    messagesQuery.clear();
    mutexQuery.unlock();
    // �믮������ �����
    if( !_makeLoggerRequest() )
    {
        qDebug() << " _writeMessQToDB: Failed to insert in  LOG query" << endl;
        mutexLogDB.unlock();

        sqlRequest.clear();
        return false;
    }
    sync();
    sqlRequest.clear();
    mutexLogDB.unlock();
    // -----------------------------------
    return true;
}


//===================================================================================
// ��⮤ 㤠����� �� �� ���� nRecords ����� ����ᥩ(���� �� ���)
bool Log_DB::_deleteFromLogDB   (int nRecords)
{
    if(nRecords < 0)
    {
        return false;
    }
    // -----------------------------------
    // ����稢��� ��⥪ᮬ ���饭�� � ��
    mutexLogDB.lock();
    if(loggerDBSize > maxDBSize)
    {
        sqlRequest = "DELETE FROM LOG      WHERE ID IN ( SELECT ID FROM LOG      DESC LIMIT " + to_string(nRecords) + ");";
    }
    else if(timeLoggerDBSize > maxDBSize)
    {
        sqlRequest+= "DELETE FROM TIME_LOG WHERE ID IN ( SELECT ID FROM TIME_LOG DESC LIMIT " + to_string(nRecords) + ");";
    }
    else if(fdLoggerDBSize > maxDBSize)
    {
        sqlRequest+= "DELETE FROM FD_LOG WHERE ID IN ( SELECT ID FROM FD_LOG DESC LIMIT " + to_string(nRecords) + ");";
    }
    else if(pulseDBSize > maxDBSize)
    {
        sqlRequest+= "DELETE FROM PULSE_LOG WHERE ID IN ( SELECT ID FROM PULSE_LOG DESC LIMIT " + to_string(nRecords) + ");";
    }
    if( !_makeLoggerRequest() )// �믮������ �����
    {
        qDebug() << " _deleteFromLogDB: ERROR!!!! " << endl;
        mutexLogDB.unlock();
        return false;
    }
    sync();
    mutexLogDB.unlock();
    // -----------------------------------
    // ���㠫����㥬 ࠧ��� ��
    _sizeOfLogDB();

    return true;
}


//===================================================================================
// ��⮤, �������騩 ࠧ��� ⠡���� ����
int Log_DB::_sizeOfLogDB()
{
    // -----------------------------------
    // ����稢��� ��⥪ᮬ ���饭�� � ��
    lock_guard<mutex> locker(mutexLogDB);
    //time log
    sqlRequest = "SELECT COUNT(*) FROM TIME_LOG;";
    if( !_makeLoggerRequest() )// �믮������ �����
    {
        qDebug() << " _sizeOfLogDB TIME_LOG: ERROR " << endl;
        return -1;
    }
    timeLoggerDBSize = loggerDBSize;
    // fd log
    sqlRequest = "SELECT COUNT(*) FROM FD_LOG;";
    if( !_makeLoggerRequest() )// �믮������ �����
    {
        qDebug() << " _sizeOfLogDB FD_LOG: ERROR " << endl;
        return -1;
    }
    fdLoggerDBSize = loggerDBSize;
    // pulse log
    sqlRequest = "SELECT COUNT(*) FROM PULSE_LOG;";
    if( !_makeLoggerRequest() )// �믮������ �����
    {
        qDebug() << " _sizeOfLogDB PULSE_LOG: ERROR " << endl;
        return -1;
    }
    pulseDBSize = loggerDBSize;
    // common log
    sqlRequest = "SELECT COUNT(*) FROM LOG;";
    if( !_makeLoggerRequest() )// �믮������ �����
    {
        cout << " _sizeOfLogDB LOG: ERROR " << endl;
        return -1;
    }
    // -----------------------------------
    if (loggerDBSize > 0)
    {
        //cout << " _sizeOfLogDB:CONFIG DB SIZE IS = " << loggerDBSize << endl;
    }
    else
    {
        cout << " Log DB SIZE IS = 0, It's emply!" << endl;
    }

    return loggerDBSize;
}


//===================================================================================
// ��⮤, �������騩 ࠧ��� ⠡���� ����
int Log_DB::_countOfTables()
{
    // -----------------------------------
    // ����稢��� ��⥪ᮬ ���饭�� � ��
    lock_guard<mutex> locker(mutexLogDB);
    sqlRequest = "select count(name) FROM sqlite_master WHERE type='table' AND  (name='TIME_LOG' OR name='FD_LOG' OR name='LOG' OR name='PULSE_LOG');";
    if( !_makeLoggerRequest() )// �믮������ �����
    {
        qDebug() << " _countOfTables: ERROR " << endl;
        return -1;
    }
    // -----------------------------------
    if (countLoggerTables < NUM_OF_TABLES)
    {
        qDebug() << "Not Enough log Tables!" << endl;
    }

    return countLoggerTables;
}

//===================================================================================
// ��⮤, �஢�ન ������� ��  ⠡���(��) ������
bool Log_DB::_isLogDBExist()
{
    countLoggerTables = _countOfTables();
    if(countLoggerTables == NUM_OF_TABLES)
    {
        return true;
    }
    else
    {
        qDebug() << " Log DB haven't got all tables, countLoggerTables = " << (int)countLoggerTables << endl;
        return false;
    }

}


//===================================================================================
// ��ॢ�� �� char* � String
string _charToString(const char *source)
{
    int size = strlen(source);
    string str(source, size);
    return str;
}


int                pInt    =    0;
EncodeConvertor    ecStr;
bool               isCP866 = true;  // ���� ⮣�, �� ⥪�� ����㯠�� � ������ � 866 ����஢��

//===================================================================================
// ��ࠡ�⪠ ��ப� ��� ������᭮�� ����饭�� � sql �����
string _prepareMess(string &sourceStr )
{
    char *safeMess;
    mutexLogDB.lock();
    if (isCP866)
    {
        pInt = 0;
        sourceStr = ecStr.CP866toUTF8(sourceStr);
    }
    safeMess =  sqlite3_mprintf("%q", sourceStr.c_str());
    string tmp     = _charToString(safeMess);
    sqlite3_free(safeMess);
    mutexLogDB.unlock();
    return tmp;
}

void Log_DB::setTermColor(LOG_LEVELS lvl)
{
    switch (lvl)
    {
        case ERROR_L:
        {
            cout << termcolor::on_red;
            break;
        }
        case WARNING_L:
        {
            cout << termcolor::underline << termcolor::yellow;
            break;
        }
        case INFO_L:
        {
            cout << termcolor::green;
            break;
        }
        case DEBUG_L:
        {
            cout << termcolor::cyan;
            break;
        }
        default:
        {
            cout << termcolor::reset;
            break;
        }
    }
}
//===================================================================================
// ������ ���� ᮮ�饭�� � ���(�� 䠪�� ���������� � ��।� ��� ��᫥���饣� �����஢����)
bool Log_DB::_log_in_sql( LOG_LEVELS lvl, LOG_REGIONS region, string mess )
{
    if(!_work.load())
    {
        return true;
    }
    //cout << "_log_in_sql: query size = " <<  messagesQuery.size() << endl;
    // �᫨ ࠧ��� ��।� �� ������ � �� ࠢ�� ���ᨬ��쭮��, � �� ������塞 ����� ᮮ�饭��
    if( messagesQuery.size() >= maxQuerySize )
    {
        qDebug() << "Log_DB::_log_in_sql(): ERROR!!! Too much messages in Query!" << endl;
        return false;
    }
    // ��ନ�㥬 �������� ����� �����(1-�� ᮮ�饭��, ��� �஢��� � �������, � ���ன �⭮���� ᮮ�饭��)
    LOG_MESSAGE                       tmpMess;
    tmpMess.mess         = _prepareMess(mess);
    tmpMess.regionOfMess =             region;
    tmpMess.levelOfMess  =                lvl;

    // ������塞 ������ � ��।� ��� ��⥪ᮬ
    mutexQuery.lock();
    messagesQuery.push_back(tmpMess);
    // �뢮� ᮮ�饭�� �� �࠭

    setTermColor(lvl);
    qDebug() << tmpMess.mess.c_str() << termcolor::reset << " " << endl;
    mutexQuery.unlock();

    return true;
}

void Log_DB::stopLogger()
{
    _work.store(false);
}

//===================================================================================
//===================================================================================
//===================================================================================
//===================================================================================
//===================================================================================
//===================================================================================
//============================        DAEMON        =================================
//===================================================================================
//===================================================================================
//===================================================================================

//===================================================================================
// ����� �ᨭ�஭��� ����� � ��
void runLogDaemon()
{
    logger.logDaemon();
}

void stopLogDaemon()
{
    logger.stopLogger();
}

//===================================================================================
//===================================================================================
//============================   C  ����������    ===================================
//===================================================================================
//===================================================================================
#ifdef __cplusplus
extern "C" {
#endif

//===================================================================================
void logINFO_c (LOG_REGIONS  region, const char *const  fmt, ... )
{
    if(logger.getLogLevel()< LOG_LEVELS::INFO_L)
        {return;}
    va_list args;
    va_start(args, fmt);
    vsnprintf ( mess, MAX_MESS_SIZE - 1, fmt, args );
    va_end(args);
    logger.log_INFO(region, mess );
}


//===================================================================================
//
void logWARN_c (LOG_REGIONS  region, const char *const  fmt, ... )
{
    if(logger.getLogLevel()< LOG_LEVELS::WARNING_L)
        {return;}
    va_list args;
    va_start(args, fmt);
    vsnprintf ( mess, MAX_MESS_SIZE - 1, fmt, args );
    va_end(args);
    logger.log_WARN(region, mess );
}


//===================================================================================
//
void logERR_c (LOG_REGIONS  region, const char *const  fmt, ... )
{
    if(logger.getLogLevel()< LOG_LEVELS::ERROR_L)
        {return;}
    va_list args;
    va_start(args, fmt);
    vsnprintf ( mess, MAX_MESS_SIZE - 1, fmt, args );
    va_end(args);
    logger.log_ERR(region, mess );
}


//===================================================================================
//
void logDBG_c (LOG_REGIONS  region, const char *const  fmt, ... )
{
    if(logger.getLogLevel()< LOG_LEVELS::DEBUG_L)
        {return;}
//    sprintf( fmt_new,
//             "%.*s",
//             MAX_MESS_SIZE-2,  fmt );

    va_list args;
    va_start(args, fmt);
//    vsprintf ( mess,fmt_new, args );
    vsnprintf ( mess, MAX_MESS_SIZE - 1, fmt, args );
    //cout << " logDBG_c():: Mess to LOGGER: size = " << sizeof(mess) << " mess=|" << mess << "|" << endl;
    va_end(args);
    logger.log_DBG(region, mess );
}


//===================================================================================
// ������/������ �஢��� ����
void          setLogLevel_c (LOG_LEVELS lvl)     {   logger.setLogLevel(lvl);    printf("--------> setLogLevel_c():: LOG LEVEL = %d\n", logger.getLogLevel());      };
LOG_LEVELS getLogLevel_c                  ()     {   return logger.getLogLevel();      };


//===================================================================================
// ������ ����஢�� �室���� ⥪��
void   setCode_CP866_c() {isCP866 = true;  };
void   setCode_UTF8_c()  {isCP866 = false; };

//===================================================================================
// ������/������ ��� �� ����
void   setDBName_c(const char      *name) {logger.setDBName( _charToString(name) ); };
//string getDBName_c                () { return logger.getDBName();                   };


//===================================================================================
// ������/������ ���� ࠧ��� ����� ��।� ᮮ�饭�� �� ������ � ��
void         setMaxQuerySize_c(unsigned int size) { logger.setMaxQuerySize(size);    };
unsigned int getMaxQuerySize_c                 () { return logger.getMaxQuerySize(); };


//===================================================================================
// ������/������ ��ਮ� ����� � ���� ��।� ᮮ�饭��
void     setWriteDBPeriod_c(unsigned usPer) { logger.setWriteDBPeriod(usPer);       };
unsigned getWriteDBPeriod_c            ()   { return logger.getWriteDBPeriod();     };


//===================================================================================
// ������/������ ���� �᫮ ����ᥩ � ��
void         setMaxDBSize_c(unsigned sz) { logger.setMaxDBSize(sz);                 };
unsigned int getMaxDBSize_c           () { return logger.getMaxDBSize();            };


#ifdef __cplusplus
}
#endif
