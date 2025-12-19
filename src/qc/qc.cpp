extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>

	#include "liblua/src/lzio.h"
	#include "liblua/src/llex.h"
	#include "liblua/src/lstring.h"
}

#include <algorithm>
#include <iostream>
#include <set>
#include <map>
#include <fstream>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

#ifndef OS_WINDOWS
#include <unistd.h>
#else
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "crc32.h"

#define OUTPUT_FOLDER "object"

using namespace std;

lua_State* L;

typedef struct LoadF {
	FILE *f;
	char buff[LUAL_BUFFERSIZE];
} LoadF;

char * g_filename;

unsigned int get_string_crc(const std::string & str)
{
	const unsigned char * s = (const unsigned char*) str.c_str();
	const unsigned char * end = s + str.size();
	unsigned int h = 0;

	while (s < end)
	{
		h *= 16777619;
		h ^= (unsigned char) *(unsigned char *) (s++);
	}

	return h;
}


static const char *getF (lua_State *L, void *ud, size_t *size) {
	LoadF *lf = (LoadF *)ud;

	if (feof(lf->f))
		return NULL;

	*size = fread(lf->buff, 1, LUAL_BUFFERSIZE, lf->f);
	return (*size > 0) ? lf->buff : NULL;
}

static void next (LexState * ls)
{
	ls->lastline = ls->linenumber;

	if (ls->lookahead.token != TK_EOS)
	{
		/* is there a look-ahead token? */
		ls->t = ls->lookahead;  /* use this one */
		ls->lookahead.token = TK_EOS;  /* and discharge it */
	}
	else
		ls->t.token = luaX_lex(ls, &ls->t.seminfo);  /* read next token */
}

static bool testnext (LexState * ls, int c)
{
	if (ls->t.token == c) 
	{
		next(ls);
		return true;
	}
	else 
		return false;
}

static void lookahead (LexState * ls)
{
	lua_assert(ls->lookahead.token == TK_EOS);
	ls->lookahead.token = luaX_lex(ls, &ls->lookahead.seminfo);
}

enum parse_state
{
	ST_START,
	ST_QUEST,
	ST_QUEST_WITH_OR_BEGIN,
	ST_STATELIST,
	ST_STATE_NAME,
	ST_STATE_BEGIN,
	ST_WHENLIST_OR_FUNCTION,
	ST_WHEN_NAME,
	ST_WHEN_WITH_OR_BEGIN,
	ST_WHEN_BODY,
	ST_FUNCTION_NAME,
	ST_FUNCTION_ARG,
	ST_FUNCTION_BODY,
};

//#define t2obj(tok) ((tok.token == TK_NAME || tok.token == TK_STRING)?getstr(tok.seminfo.ts):((tok.token==TK_NUMBER)?lexstate.t.seminfo.r:luaX_token2str(&lexstate,tok.token)))

struct LexState * pls = 0;
void errorline(int line , const char * str)
{
	cout.flush();
	if (g_filename)
		cerr << g_filename <<":";
	cerr << line<< ':';
	cerr << str << endl;
	abort();
}
void error(const char * str)
{
	cout.flush();
	if (g_filename)
		cout << g_filename <<":";
	if (pls)
	{
		cout << pls->linenumber << ':';
	}
	cerr << str << endl;
	abort();
}

#define assert(exp) if (!(exp)) error("assertion failure : " #exp)
#define assert_msg(exp,msg) if (!(exp)) error(msg " : " #exp)

ostream & operator << (ostream & ostr, const Token& tok)
{
	if (tok.token == TK_NAME)
		ostr << getstr(tok.seminfo.ts);
	else if (tok.token == TK_NUMBER)
		ostr << tok.seminfo.r;
	else if (tok.token == TK_STRING)
		ostr << '"' << getstr(tok.seminfo.ts) <<'"';
	else
		ostr << luaX_token2str(pls,tok.token);
	return ostr;
}

bool check_syntax(const string& str, const string& module)
{
	int ret = luaL_loadbuffer(L, str.c_str(), str.size(), module.c_str());

	if (ret)
	{
		cerr << str << endl;
		error((string("syntax error : ")+lua_tostring(L,-1) ).c_str());
		return false;
	}

	lua_pop(L,1);
	return true;
}

int none_c_function(lua_State* L)
{
	return 0;
}

set<string> function_defs;
set<string> function_calls;

void RegisterDefFunction(const string& fname)
{
	function_defs.insert(fname);
}

void RegisterUsedFunction(const string& fname)
{
	function_calls.insert(fname);
}

void CheckUsedFunction()
{
	bool hasError = false;
	set<string> error_func;

	for (auto it = function_calls.begin(); it != function_calls.end(); ++it)
	{
		if (function_defs.find(*it) == function_defs.end())
		{
			hasError = true;
			error_func.insert(*it);
		}
		//cout << "Used : " << *it <<  endl;
	}

	if (hasError)
	{
		cout << "Calls undeclared function! : " << endl;
		for (auto it = error_func.begin(); it != error_func.end(); ++it)
		{
			cout << *it << endl;
		}
		abort();
	}
}

void load_quest_function_list(const char* filename)
{
	ifstream inf(filename);
	string s;

	while (!inf.eof())
	{
		inf >> s;
		if (inf.fail())
			break;

		RegisterDefFunction(s);
	}
}

struct AScript
{
	string when_condition;
	string when_argument;
	string script;

	AScript(string a, string b, string c) :
		when_condition(a),
		when_argument(b),
		script(c)
	{}

	AScript() {}
};

void parse(char * filename)
{
	LoadF lf;
	lf.f = fopen(filename,"r");
	ZIO z;
	luaZ_init(&z, getF,&lf,"quest");
	Mbuffer b;
	struct LexState lexstate;
	pls = &lexstate;
	luaZ_initbuffer(L,&b);
	lexstate.buff = &b;
	luaX_setinput(L, &lexstate, &z, luaS_new(L, zname(&z)));

	parse_state ps = ST_START;

	int nested = 0;

	string quest_name;
	string start_condition;

	string current_state_name;
	string current_when_name;
	string current_when_condition;

	string current_when_argument;

	set<string> define_state_name_set;
	map<int,string> used_state_name_map;

	map<string, map<string, string> > state_script_map;
	map<string, map<string, vector<AScript> > > state_arg_script_map;

	vector<pair<string, string> > when_name_arg_vector;

	string current_function_name;
	string current_function_arg;
	string all_functions;

	load_quest_function_list("quest_functions");

	while (1)
	{
		next(&lexstate);

		/*/
		  cout << luaX_token2str(&lexstate,lexstate.t.token);
		  if (lexstate.t.token == TK_NAME)
		  cout << '\t' << getstr(lexstate.t.seminfo.ts);
		  else if (lexstate.t.token == TK_NUMBER)
		  cout << '\t' << lexstate.t.seminfo.r;
		  else if (lexstate.t.token == TK_STRING)
		  cout << '\t' << '"' << getstr(lexstate.t.seminfo.ts) <<'"';
		  cout << endl;
		//*/

		if (lexstate.t.token == TK_EOS) break;

		Token& t= lexstate.t;

		switch(ps)
		{
			case ST_START:
				{
					assert(nested==0);
					if (t.token == TK_QUEST)
						ps = ST_QUEST;
					else
						error("must start with 'quest'");
				}
				break;
			case ST_QUEST:
				{
					assert(nested==0);
					if (t.token == TK_NAME || t.token == TK_STRING)
					{
						quest_name = getstr(lexstate.t.seminfo.ts);
						cout << "QUEST : " << quest_name << endl;
						ps = ST_QUEST_WITH_OR_BEGIN;
					}
					else
						error("quest name must be given");
				}
				break;
			case ST_QUEST_WITH_OR_BEGIN:
				assert(nested==0);
				if (t.token == TK_WITH)
				{
					assert(nested==0);
					next(&lexstate);
					ostringstream os;
					os << (lexstate.t);
					//cout << (lexstate.t);
					next(&lexstate);
					while (lexstate.t.token!=TK_DO)
					{
						os << " " <<(lexstate.t);
						//cout << TK_DO<<lexstate.t.token << " " <<(lexstate.t) <<endl;
						next(&lexstate);
					}
					start_condition = os.str();
					check_syntax("if "+start_condition+" then end", quest_name);
					cout << "\twith ";
					cout << start_condition;
					cout << endl;
					t = lexstate.t;
				}
				if (t.token == TK_DO)
				{
					ps = ST_STATELIST;
					nested++;
				}
				else
				{
					ostringstream os;
					os << "quest doesn't have begin-end clause. (" << t << ")";
					error(os.str().c_str());
				}
				break;
			case ST_STATELIST:
				{
					assert(nested==1);
					if (t.token == TK_STATE)
					{
						ps = ST_STATE_NAME;
					}
					else if (t.token == TK_END)
					{
						nested --;
						ps = ST_START;
					}
					else
					{
						error("expecting 'state'");
					}
				}
				break;
			case ST_STATE_NAME:
				{
					assert(nested==1);
					if (t.token == TK_NAME || t.token == TK_STRING)
					{
						current_state_name = getstr(t.seminfo.ts);
						define_state_name_set.insert(current_state_name);
						cout << "STATE : " << current_state_name << endl;
						ps = ST_STATE_BEGIN;
					}
					else
					{
						error("state name must be given");
					}
				}
				break;

			case ST_STATE_BEGIN:
				{
					assert(nested==1);

					if (t.token == TK_DO)
					{
						nested ++;
						ps = ST_WHENLIST_OR_FUNCTION;
					}
					else
					{
						error("state doesn't have begin-end clause.");
					}
				}
				break;

			case ST_WHENLIST_OR_FUNCTION:
				{
					assert(nested==2);

					if (t.token == TK_WHEN)
					{
						ps = ST_WHEN_NAME;

						when_name_arg_vector.clear();
					}
					else if (t.token == TK_END)
					{
						nested--;
						ps = ST_STATELIST;
					}
					else if (t.token == TK_FUNCTION)
					{
						ps = ST_FUNCTION_NAME;
					}
					else
					{
						error("expecting 'when' or 'function'");
					}
				}
				break;

			case ST_WHEN_NAME:
				{
					assert(nested==2);
					if (t.token == TK_NAME || t.token == TK_STRING || t.token == TK_NUMBER)
					{
						if (t.token == TK_NUMBER)
						{
							ostringstream os;
							os << (unsigned int)t.seminfo.r;
							current_when_name = os.str();
							lexstate.lookahead.token  = '.';
						}
						else
						{
							current_when_name = getstr(t.seminfo.ts);
							lookahead(&lexstate);
						}
						ps = ST_WHEN_WITH_OR_BEGIN;
						current_when_argument = "";
						if (lexstate.lookahead.token == '.')
						{
							next(&lexstate);
							current_when_name += '.';
							next(&lexstate);
							ostringstream os;
							t = lexstate.t;
							os << t;
							if (os.str() == "target")
							{
								current_when_argument = "." + current_when_name;
								current_when_argument.resize(current_when_argument.size()-1);
								current_when_name = "target";
							}
							else
							{
								current_when_name += os.str();
							}
							lookahead(&lexstate);
						}

						{
							// make when argument
							ostringstream os;
							while (lexstate.lookahead.token == '.')
							{
								next(&lexstate);
								os << '.';
								next(&lexstate);
								t = lexstate.t;
								//if (t.token == TK_STRING)
								//t.token = TK_NAME;
								os << t;
								lookahead(&lexstate);
							}
							current_when_argument += os.str();
						}
						cout << "WHEN  : " << current_when_name ;
						if (!current_when_argument.empty())
						{
							cout << " (";
							cout << current_when_argument.substr(1);
							cout << ")";
						}
					}
					else
					{
						error("when name must be given");
					}

					if (lexstate.lookahead.token == TK_OR)
					{
						// ���� when name
						// push to somewhere -.-
						ps = ST_WHEN_NAME;
						when_name_arg_vector.push_back(make_pair(current_when_name, current_when_argument));

						next(&lexstate);
						cout << " or" << endl;
					}
					else
					{
						cout <<endl;
					}
				}
				break;
			case ST_WHEN_WITH_OR_BEGIN:
				{
					assert(nested==2);
					current_when_condition = "";
					if (t.token == TK_WITH)
					{
						// here comes ���ǽ�
						next(&lexstate);
						ostringstream os;
						os << (lexstate.t);
						//cout << (lexstate.t);
						next(&lexstate);
						while (lexstate.t.token!=TK_DO)
						{
							os << " " <<(lexstate.t);
							//cout << TK_DO<<lexstate.t.token << " " <<(lexstate.t) <<endl;
							next(&lexstate);
						}
						current_when_condition = os.str();
						check_syntax("if "+current_when_condition+" then end", current_state_name+current_when_condition);
						cout << "\twith ";
						cout << current_when_condition;
						cout << endl;
						t = lexstate.t;
					}
					if (t.token == TK_DO)
					{
						ps = ST_WHEN_BODY;
						nested++;
					}
					else
					{
						//error("when doesn't have begin-end clause.");
						ostringstream os;
						os << "when doesn't have begin-end clause. (" << t << ")";
						error(os.str().c_str());
					}

				}
				break;
			case ST_WHEN_BODY:
				{
					assert(nested==3);

					// output
					ostringstream os;
					int state_check = 0;
					auto prev = lexstate;
					string callname;
					bool registered = false;
					if (prev.t.token == '.')
						prev.t.token = TK_DO; // any token
					while (1)
					{
						if (lexstate.t.token == TK_DO || lexstate.t.token == TK_IF /*|| lexstate.t.token == TK_FOR*/ || lexstate.t.token == TK_BEGIN || lexstate.t.token == TK_FUNCTION)
						{
							//cout << ">>>" << endl;
							nested++;
						}
						else if (lexstate.t.token == TK_END)
						{
							//cout << "<<<" << endl;
							nested--;
						}

						if (!callname.empty())
						{
							lookahead(&lexstate);
							if (lexstate.lookahead.token == '(')
							{
								RegisterUsedFunction(callname);
								registered = true;
							}
							callname.clear();
						}
						else if (lexstate.t.token == '(')
						{ 
							if (!registered && prev.t.token == TK_NAME)
								RegisterUsedFunction(getstr(prev.t.seminfo.ts));
							registered = false;
						}

						if (lexstate.t.token == '.')
						{
							ostringstream fname;
							lookahead(&lexstate);
							fname << prev.t << '.' << lexstate.lookahead;
							callname = fname.str();
						}

						if (state_check)
						{
							state_check--;
							if (!state_check)
							{
								if (lexstate.t.token == TK_NAME || lexstate.t.token == TK_STRING)
								{
									used_state_name_map[lexstate.linenumber] = (getstr(lexstate.t.seminfo.ts));
									lexstate.t.token = TK_STRING;
								}
							}
						}

						if (lexstate.t.token == TK_NAME && ((strcmp(getstr(lexstate.t.seminfo.ts),"set_state")==0) || (strcmp(getstr(lexstate.t.seminfo.ts),"newstate")==0) || (strcmp(getstr(lexstate.t.seminfo.ts),"setstate")==0)))
						{
							state_check = 2;
						}
						if (nested==2) break;
						os << lexstate.t << ' ';
						prev = lexstate;
						next(&lexstate);
						if (lexstate.linenumber != lexstate.lastline)
							os << endl;
					}


					//cout << os.str() << endl;

					check_syntax(os.str(), current_state_name+current_when_condition);
					reverse(when_name_arg_vector.begin(), when_name_arg_vector.end());
					while (1)
					{
						if (current_when_argument.empty())
						{
							if (current_when_condition.empty())
								state_script_map[current_when_name][current_state_name]+=os.str();
							else
								state_script_map[current_when_name][current_state_name]+="if "+current_when_condition+" then "+os.str()+" return end ";
						}
						else
						{
							state_arg_script_map[current_when_name][current_state_name].push_back(AScript(current_when_condition, current_when_argument, os.str()));
						}

						if (!when_name_arg_vector.empty())
						{
							current_when_name = when_name_arg_vector.back().first;
							current_when_argument = when_name_arg_vector.back().second;
							when_name_arg_vector.pop_back();
						}
						else
							break;
					}

					ps = ST_WHENLIST_OR_FUNCTION;
				}
				break;
			case ST_FUNCTION_NAME:
				if (t.token == TK_NAME)
				{
					current_function_name = getstr(t.seminfo.ts);
					RegisterDefFunction(quest_name+"."+current_function_name);
					ps = ST_FUNCTION_ARG;
				}
				break;
			case ST_FUNCTION_ARG:
				{
					assert(t.token == '(');
					next(&lexstate);
					current_function_arg = '(';
					if (t.token != ')')
					{
						do 
						{
							if (t.token == TK_NAME)
							{
								current_function_arg += getstr(t.seminfo.ts);
								next(&lexstate);
								if (t.token != ')')
									current_function_arg += ',';
							}
							else
							{
								ostringstream os;
								os << "invalud argument name " << getstr(t.seminfo.ts) << " for function " << current_function_name;
								error(os.str().c_str());
							}
						} while (testnext(&lexstate, ','));
					}
					current_function_arg += ')';
					ps = ST_FUNCTION_BODY;
					nested ++;
				}
				break;

			case ST_FUNCTION_BODY:
				{
					assert(nested == 3);
					ostringstream os;
					auto prev = lexstate;
					bool registered = false;
					if (prev.t.token == '.')
						prev.t.token = TK_DO;
					string callname;
					while (nested >= 3)
					{
						if (lexstate.t.token == TK_DO || lexstate.t.token == TK_IF /*|| lexstate.t.token == TK_FOR*/ || lexstate.t.token == TK_BEGIN || lexstate.t.token == TK_FUNCTION)
						{
							//cout << ">>>" << endl;
							nested++;
						}
						else if (lexstate.t.token == TK_END)
						{
							//cout << "<<<" << endl;
							nested--;
						}

						if (!callname.empty())
						{
							lookahead(&lexstate);
							if (lexstate.lookahead.token == '(')
							{
								RegisterUsedFunction(callname);
								registered = true;
							}
							callname.clear();
						}
						else if (lexstate.t.token == '(')
						{ 
							if (!registered && prev.t.token == TK_NAME)
								RegisterUsedFunction(getstr(prev.t.seminfo.ts));
							registered = false;
						}

						if (lexstate.t.token == '.')
						{
							ostringstream fname;
							lookahead(&lexstate);
							fname << prev.t << '.' << lexstate.lookahead;
							callname = fname.str();
						}

						os << lexstate.t << ' ';
						if (nested == 2)
							break;
						prev = lexstate;
						next(&lexstate);
						//cout << lexstate.t << ' ' << lexstate.linenumber << ' ' << lexstate.lastline << endl;
						if (lexstate.linenumber != lexstate.lastline)
							os << endl;

					}
					ps = ST_WHENLIST_OR_FUNCTION;
					all_functions += ',';
					all_functions += current_function_name;
					all_functions += "= function ";
					all_functions += current_function_arg;
					all_functions += os.str();
					cout << "FUNCTION " << current_function_name << current_function_arg << endl;
				}
				break;

		} // end of switch

	}
	assert(nested==0);
	for (map<int,string>::iterator it = used_state_name_map.begin();it!=used_state_name_map.end();++it)
	{
		if (define_state_name_set.find(it->second)==define_state_name_set.end())
		{
			errorline(it->first,("state name not found : "+it->second).c_str());
		}
	}

	if (!define_state_name_set.empty())
	{
		if (0 != mkdir(OUTPUT_FOLDER "/state", S_IRWXU))
		{
			if (errno != EEXIST)
			{
				perror("cannot create directory");
				exit(1);
			}
		}

		ofstream ouf((string(OUTPUT_FOLDER "/state/")+quest_name).c_str());
		ouf << quest_name << "={[\"start\"]=0";
		set<string> :: iterator it;

		map<string, int> state_crc;
		set<int> crc_set;

		state_crc["start"] = 0;
		for (it = define_state_name_set.begin(); it != define_state_name_set.end(); ++it)
		{
			int crc = (int) CRC32((*it).c_str());

			if (crc_set.find(crc) == crc_set.end())
			{
				crc_set.insert(crc);	
			}
			else
			{
				++crc;

				while (crc_set.find(crc) != crc_set.end()) 
					++crc;

				printf("WARN: state CRC conflict occur! state index may differ in next compile time.\n");
				crc_set.insert(crc);
			}
			state_crc.insert(make_pair(*it, crc));
		}

		int idx = 1;

		for (it = define_state_name_set.begin(); it!=define_state_name_set.end(); ++it)
		{
			if (*it != "start")
			{
				ouf << ",[\"" << *it << "\"]=" <<state_crc[*it];
				++idx;
			}
		}

		// quest function���� ���
		ouf << all_functions;

		ouf << "}";
	}

	if (!start_condition.empty())
	{
		if (0 != mkdir(OUTPUT_FOLDER "/begin_condition", S_IRWXU))
		{
			if (errno != EEXIST)
			{
				perror("cannot create directory");
				exit(1);
			}
		}

		ofstream ouf((string(OUTPUT_FOLDER "/begin_condition/")+quest_name).c_str());

		ouf << "return " <<start_condition;
		ouf.close();
	}

	{
		map<string, map<string, vector<AScript> > >::iterator it;

		for (it = state_arg_script_map.begin(); it != state_arg_script_map.end(); ++it)
		{
			string second_name;
			string path;

			if (it->first.find('.')==it->first.npos)
			{
				// one like login
				string s(it->first);
				transform(s.begin(),s.end(),s.begin(),::tolower);
				mkdir(OUTPUT_FOLDER "/notarget",0755);
				mkdir((OUTPUT_FOLDER "/notarget/"+s).c_str(),0755);
				path = OUTPUT_FOLDER "/notarget/"+s+"/";
				second_name = s;
			}
			else
			{
				// two like [WHO].Kill
				string s = it->first;
				transform(s.begin(),s.end(),s.begin(),::tolower);
				int i = s.find('.');
				mkdir((OUTPUT_FOLDER "/"+it->first.substr(0,i)).c_str(),0755);
				mkdir((OUTPUT_FOLDER "/"+it->first.substr(0,i)+"/"+s.substr(i+1,s.npos)).c_str(),0755);
				path = OUTPUT_FOLDER "/" + it->first.substr(0,i) + "/"+s.substr(i+1,s.npos)+"/";
				second_name = s.substr(i+1,s.npos);
			}

			map<string,vector<AScript> >::iterator it2;
			for (it2 = it->second.begin(); it2 != it->second.end(); ++it2)
			{
				for (int i = 0; i < it2->second.size(); ++i)
				{
					ostringstream os;
					os << i;
					{
						ofstream ouf((path+quest_name+"."+it2->first+"."+os.str()+"."+"script").c_str());
						copy(it2->second[i].script.begin(),it2->second[i].script.end(), ostreambuf_iterator<char>(ouf));
					}
					{
						ofstream ouf((path+quest_name+"."+it2->first+"."+os.str()+"."+"when").c_str());
						if (!it2->second[i].when_condition.empty())
						{
							ouf << "return ";
							copy(it2->second[i].when_condition.begin(),it2->second[i].when_condition.end(), ostreambuf_iterator<char>(ouf));
						}
					}
					{
						ofstream ouf((path+quest_name+"."+it2->first+"."+os.str()+"."+"arg").c_str());
						copy(it2->second[i].when_argument.begin()+1,it2->second[i].when_argument.end(), ostreambuf_iterator<char>(ouf));
					}
				}
			}
		}
	}

	{
		map<string,map<string,string> >::iterator it;

		for (it = state_script_map.begin();it!=state_script_map.end();++it)
		{
			string path;
			if (it->first.find('.')==it->first.npos)
			{
				// one
				string s = it->first;
				transform(s.begin(),s.end(),s.begin(),::tolower);
				mkdir(OUTPUT_FOLDER "/notarget",0755);
				mkdir((OUTPUT_FOLDER "/notarget/"+s).c_str(),0755);
				path = OUTPUT_FOLDER "/notarget/"+s+"/";
			}
			else
			{
				// two like [WHO].Kill
				string s = it->first;
				transform(s.begin(),s.end(),s.begin(),::tolower);
				int i = s.find('.');
				mkdir((OUTPUT_FOLDER "/"+it->first.substr(0,i)).c_str(),0755);
				mkdir((OUTPUT_FOLDER "/"+it->first.substr(0,i)+"/"+s.substr(i+1,s.npos)).c_str(),0755);
				path = OUTPUT_FOLDER "/" + it->first.substr(0,i) + "/"+s.substr(i+1,s.npos)+"/";
			}

			map<string,string>::iterator it2;
			for (it2 = it->second.begin();it2!=it->second.end();++it2)
			{
				ofstream ouf((path+quest_name+"."+it2->first).c_str());
				copy(it2->second.begin(),it2->second.end(), ostreambuf_iterator<char>(ouf));
			}
		}
	}

	CheckUsedFunction();    
}

int main(int argc, char* argv[])
{
	mkdir(OUTPUT_FOLDER, 0700);
	L = lua_open();
	luaX_init(L);

	if (argc > 1)
	{
		for (int i = 1; i < argc; ++i)
		{
			g_filename = argv[i];
			parse(argv[i]);
		}
	}

	lua_close(L);
	return 0;
}

