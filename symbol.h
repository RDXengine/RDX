#ifndef _SYMBOL_H_
#define _SYMBOL_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <iostream>
#include <algorithm>
#include <string>
#include <string.h>
#include <memory>
#include <map>
#include <set>
#include <queue>
#include <stack>
#include <vector>
#include <stdio.h>
#include <ctype.h>

#include "../tools.h"

using namespace std;

class Symbol;

typedef ref_ptr<Symbol> symPtr;

class Symbol
{
	public:
		Symbol(string inName) : name(inName), primative(false) {}
		Symbol(string inName, double inVal)
			: name(inName), dblVal(inVal), primative(true), isNum(true) {}
		Symbol(string inName, string inVal)
			: name(inName), strVal(inVal), primative(true), isNum(false), dblVal(0) {}
		string str() const
		{
			string ret = name;
			if (primative)
			{
				if (isNum)
				{
					char buf[32];
					sprintf(buf, "=%.2f", dblVal);
					ret += buf;
				}
				else
				{
					ret += string("=\"") + strVal + "\"";
				}
			}
			else
			{
				ret += ": {";
				for (vector<symPtr>::const_iterator ii=members.begin(); ii != members.end(); ii++)
				{
					if (ii != members.begin()) {ret += ", ";}
					ret += (*ii)->str();
				}
				ret += "}";
			}

			return ret;
		}
		void dump(int inTab) const
		{
			for (int tt=0; tt<inTab; tt++) cout << "\t";
			cout << name;

			if (primative)
			{
				if (isNum)
				{
					cout << "=" << dblVal << endl;
				}
				else
				{
					cout << "=\"" << strVal << "\"" << endl;
				}
			}
			else
			{
				cout << endl;
				for (int tt=0; tt<inTab; tt++) cout << "\t";
				cout << "{" << endl;
				for (vector<symPtr>::const_iterator ii=members.begin(); ii != members.end(); ii++)
				{
					(*ii)->dump(inTab+1);
				}
				for (int tt=0; tt<inTab; tt++) cout << "\t";
				cout << "}" << endl;
			}
		}

		string getName() const
		{
			return name;
		}

		bool isPrimitive() const
		{
			return primative;
		}

		double getDblValue() const
		{
			if ((!isNum) && (dblVal == 0))
			{
				((Symbol*) this)->dblVal = atof(strVal.c_str());
			}
			return dblVal;
		}

		string getStrValue() const
		{
			if ((isNum) && (strVal.length() < 1))
			{
				char cc[32];
				sprintf(cc, "%.2f", dblVal);
				((Symbol*) this)->strVal = string(cc);
			}
			return strVal;
		}

		void addSym(symPtr inChild)
		{
			members.push_back(inChild);
			mbrTbl[inChild->getName()].push_back(inChild);
		}

		symPtr getSymbol(string inName, int inIndx)
		{
			if ((mbrTbl.find(inName) != mbrTbl.end()) && (mbrTbl[inName].size() > inIndx))
				return mbrTbl[inName][inIndx];
			return symPtr();
		}

		symPtr getSymbol(int inIndx) const
		{
			if (members.size() > inIndx)
				return members[inIndx];
			return symPtr();
		}

		int getSymbolCount() const
		{
			return members.size();
		}

		int getSymbolCount(string inName)
		{
			return mbrTbl[inName].size();
		}

		symPtr clone() const
		{
			return new Symbol(*((Symbol*) this));
		}

private:
		string name;
		bool primative;
		bool isNum;
		double dblVal;
		string strVal;
		vector<symPtr> members;
		map<string, vector<symPtr> > mbrTbl;

		Symbol(Symbol &inSrc)
			: name(inSrc.name), dblVal(inSrc.dblVal), primative(inSrc.primative)
		{
			for (int ii=0; ii<inSrc.members.size(); ii++)
			{
				symPtr sp = inSrc.members[ii]->clone();
				members.push_back(sp);
				mbrTbl[sp->getName()].push_back(sp);
			}
		}
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class SymbolTable;

typedef ref_ptr<SymbolTable> symTblPtr;

class SymbolTable
{
	public:
		SymbolTable() : curState(0) {}
		int getState() const
		{
			return curState;
		}

		double get(string inName, int inInd)
		{
			if (kvTbl.find(inName) != kvTbl.end())
				return kvTbl[inName];
			if ((tbl.find(inName) != tbl.end()) && (tbl[inName].size() > inInd))
				return tbl[inName][inInd]->getDblValue();
			return 0;
		}

		void push(int inNewState, symPtr inSym)
		{
			stk.push_back(make_pair(curState, inSym));
			curState = inNewState;
			tbl[inSym->getName()].push_back(inSym);
		}
		symPtr pop()
		{
			string name = stk.back().second->getName();

			if (tbl[name].size() > 1)
			{
				tbl[name].pop_back();
			}
			else
			{
				tbl.erase(name);
			}
			symPtr ret = stk.back().second;
			curState = stk.back().first;
			stk.pop_back();
			return ret;
		}
		symPtr topSym()
		{
			return stk.back().second;
		}
		void setAside(int inLaCnt)
		{
			for (int ii=0; ii<inLaCnt; ii++)
			{
				symPtr sym = stk.back().second;
				laStk.push(sym);
				stk.pop_back();
				string symNm = sym->getName();
				if (tbl[symNm].size() > 1)
				{
					tbl[symNm].pop_back();
				}
				else
				{
					tbl.erase(symNm);
				}
			}
		}

		bool hasAside()
		{
			return (!laStk.empty());
		}
		symPtr popNextAside()
		{
			symPtr ret = laStk.top();
			laStk.pop();
			return ret;
		}
		int stackSize()
		{
			return stk.size();
		}

		symPtr getSymbol(string inName, int inIndex)
		{
			if ((tbl.find(inName) != tbl.end()) && (tbl[inName].size() > inIndex))
			{
				return tbl[inName][inIndex];
			}
			return symPtr();
		}

		int getSymCnt(string inName)
		{
			if (tbl.find(inName) == tbl.end()) return 0;
			return tbl[inName].size();
		}

		void dump(int inTab)
		{
			for (int tt=0; tt<inTab; tt++) cout << "\t";
			cout << "SymbolTable" << endl;
			for (int tt=0; tt<inTab; tt++) cout << "\t";
			cout << "{" << endl;

			for (map<string, vector<symPtr> >::iterator ii = tbl.begin(); ii != tbl.end(); ii++)
			{
				for (int jj=0; jj<ii->second.size(); jj++)
				{
					for (int tt=0; tt<=inTab; tt++) cout << "\t";
					cout << ii->first << "[" << jj << "]" << endl;
					ii->second[jj]->dump(inTab+2);
				}
			}

			for (int tt=0; tt<inTab; tt++) cout << "\t";
			cout << "}" << endl;
		}

		int getKV(string inKey) { return kvTbl[inKey]; }
		void setKV(string inKey, int inVal) { kvTbl[inKey] = inVal; }
		void unsetKV(string inKey) { kvTbl.erase(inKey); }
		bool hasKV(string inKey) { return (kvTbl.find(inKey) != kvTbl.end()); }

		symTblPtr fork() const
		{
			return new SymbolTable(*((SymbolTable*) this));
		}

	private:
		map<string, vector<symPtr> > tbl;
		vector<pair<int, symPtr> > stk;
		stack<symPtr> laStk;
		map<string, int> kvTbl;
		int curState;

		SymbolTable(SymbolTable& inSrc) : curState(inSrc.curState)
		{
			for (int ii=0; ii<inSrc.stk.size(); ii++)
			{
				symPtr sp = inSrc.stk[ii].second->clone();
				stk.push_back(make_pair(inSrc.stk[ii].first, sp));
				tbl[sp->getName()].push_back(sp);
			}
			kvTbl = inSrc.kvTbl;
			curState = inSrc.curState;
		}
};

#endif
