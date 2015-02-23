#ifndef _ENGINE_H
#define _ENGINE_H

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
#include "../symbol/symbol.h"
#include "../regex/regex.h"
#include "../state/state_tbl.h"

////////////////////////////////////////////////////////////////////////////////

class Expression
{
	public:
		virtual double evaluate(symTblPtr inSTbl) const =0;

		virtual symPtr getSymbol(int inIndx, symTblPtr inSTbl) const { return new Symbol(getIdentifier(), evaluate(inSTbl)); }
		virtual symPtr getSymbol(symPtr inParent, int inIndx, symTblPtr inSTbl) const { return symPtr(); }
		virtual int getSymbolCount(symTblPtr inSTbl) const =0;
		virtual int getSymbolCount(symPtr inParent, symTblPtr inSTbl) const { return 0; }

		virtual string getIdentifier() const { return ""; }

		virtual string str() const =0;
};

typedef ref_ptr<Expression> expPtr;

class Value : public Expression
{
	public:
		Value(double inV) : vv(inV) {}
		double evaluate(symTblPtr inSTbl) const
		{
			return vv;
		}
		int getSymbolCount(symTblPtr inSTbl) const { return 1; }
		string str() const { char buf[16]; sprintf(buf, "%.2f", vv); return string(buf); }
	private:
		double vv;
};

class Variable : public Expression
{
	public:
		Variable(string inName, expPtr inInd)
			: name(inName), ind(inInd), hasIndex(true) {}
		Variable(string inName)
			: name(inName), ind(new Value(0)), hasIndex(false) {}

		double evaluate(symTblPtr inSTbl) const
		{
			if (hasIndex)
			{
				int ii = ind->evaluate(inSTbl);
				return inSTbl->get(name, ii);
			}
			return inSTbl->get(name, 0);
		}

		symPtr getSymbol(int inIndx, symTblPtr inSTbl) const
		{
			if (hasIndex)
			{
				if (inIndx > 0) throw string("bug: Can't access an indexed variable ") + name + " by second index.";
				int cc = ind->evaluate(inSTbl);
				return inSTbl->getSymbol(name, cc);
			}
			return inSTbl->getSymbol(name, inIndx);
		}
		symPtr getSymbol(symPtr inParent, int inIndx, symTblPtr inSTbl) const
		{
			if (hasIndex)
			{
				if (inIndx > 0) throw string("bug: Can't access an indexed variable ") + name + " by second index.";
				int cc = ind->evaluate(inSTbl);
				return inParent->getSymbol(name, cc);
			}
			return inParent->getSymbol(name, inIndx);
		}
		int getSymbolCount(symTblPtr inSTbl) const
		{
			int cnt = inSTbl->getSymCnt(name);
			if (hasIndex)
			{
				int cc = ind->evaluate(inSTbl);
				return (cnt > cc) ? 1 : 0;
			}
			return cnt;
		}
		int getSymbolCount(symPtr inParent, symTblPtr inSTbl) const
		{
			int cnt = inParent->getSymbolCount(name);
			if (hasIndex)
			{
				int cc = ind->evaluate(inSTbl);
				return (cnt > cc) ? 1 : 0;
			}
			return cnt;
		}

		string getIdentifier() const {return name; }
		string str() const
		{
			if (hasIndex) return name + "[" + ind->str() + "]";
			return name;
		}
	private:
		string name;
		expPtr ind;
		bool hasIndex;
};

class Path : public Expression
{
	public:
		void add(expPtr inElem)
		{
			elems.push_back(inElem);
		}

		double evaluate(symTblPtr inSTbl) const
		{
			if (elems.size() == 0) throw string("Poorly formed path in eval()");

			symPtr ss = elems[0]->getSymbol(0, inSTbl);
			if (ss.isNull()) return 0;

			for (int ii=1; ii<elems.size(); ii++)
			{
				ss = elems[ii]->getSymbol(ss, 0, inSTbl);
				if (ss.isNull()) return 0;
			}

			return ss->getDblValue();
		}

		symPtr getSymbol(int inIndx, symTblPtr inSTbl) const
		{
			if (syms.size() == 0) getSymbolCount(inSTbl);
			if (syms.size() <= inIndx) throw string("Poorly formed path in getSymbol()");
			return syms[inIndx];
		}
		int getSymbolCount(symTblPtr inSTbl) const
		{
			((Path*) this)->syms.clear();

			if (elems.size() == 0) throw string("Poorly formed path in symCnt()");

			((Path*) this)->buildSyms(symPtr(), 0, inSTbl);

			return syms.size();
		}
		string getIdentifier() const {return "path"; }
		string str() const
		{
			string ret = "";
			for (int ii=0; ii<elems.size(); ii++)
			{
				if (ii > 0) ret += ".";
				ret += elems[ii]->str();
			}
			return ret;
		}
	private:
		vector<expPtr> elems;
		vector<symPtr> syms;

		void buildSyms(symPtr inSym, int inPathDepth, symTblPtr inSTbl)
		{
			int cnt = 0;
			if (inPathDepth == 0)
			{
				cnt = elems[inPathDepth]->getSymbolCount(inSTbl);
			}
			else
			{
				cnt = elems[inPathDepth]->getSymbolCount(inSym, inSTbl);
			}
			for (int ii=0; ii<cnt; ii++)
			{
				symPtr ss;

				if (inPathDepth == 0)
				{
					ss = elems[inPathDepth]->getSymbol(ii, inSTbl);
				}
				else
				{
					ss = elems[inPathDepth]->getSymbol(inSym, ii, inSTbl);
				}

				if (inPathDepth == (elems.size() - 1))
				{
					syms.push_back(ss);
				}
				else
				{
					buildSyms(ss, inPathDepth+1, inSTbl);
				}
			}
		}
};

class Each : public Expression
{
	public:
		Each(expPtr inCol, expPtr inVar)
			: col(inCol),var(inVar) {}

		double evaluate(symTblPtr inSTbl) const
		{
			throw string("Can't evaluate Each: ") + str();
		}

		symPtr getSymbol(int inIndx, symTblPtr inSTbl) const
		{
			symPtr ret = col->getSymbol(inIndx, inSTbl);
			ret = var->getSymbol(ret, 0, inSTbl);

			return ret;
		}

		int getSymbolCount(symTblPtr inSTbl) const
		{
			return col->getSymbolCount(inSTbl);
		}

		string getIdentifier() const {return "each"; }
		string str() const { return string("each (") + col->str() + ", " + var->str() + ")"; }
	private:
		expPtr col;
		expPtr var;
};

class Count : public Expression
{
	public:
		Count(expPtr inA) : aa(inA) {}
		double evaluate(symTblPtr inSTbl) const
		{
			return aa->getSymbolCount(inSTbl);
		}
		int getSymbolCount(symTblPtr inSTbl) const
		{
			return aa->getSymbolCount(inSTbl);
		}
		string getIdentifier() const {return "count"; }
		string str() const { return string("count(") + aa->str() + ")"; }
	private:
		expPtr aa;
};

class Sum : public Expression
{
	public:
		Sum(expPtr inA) : aa(inA) {}
		double evaluate(symTblPtr inSTbl) const
		{
			double ret = 0;
			int cc = aa->getSymbolCount(inSTbl);
			for (int ii=0; ii<cc; ii++)
			{
				ret += aa->getSymbol(ii, inSTbl)->getDblValue();
			}

			return ret;
		}
		int getSymbolCount(symTblPtr inSTbl) const
		{
			return aa->getSymbolCount(inSTbl);
		}
		string getIdentifier() const {return "sum"; }
		string str() const { return string("sum(") + aa->str() + ")"; }
	private:
		expPtr aa;
};

class Plus : public Expression
{
	public:
		Plus(expPtr inL, expPtr inR) : ll(inL), rr(inR) {}
		double evaluate(symTblPtr inSTbl) const
		{
			return ll->evaluate(inSTbl) + rr->evaluate(inSTbl);
		}
		int getSymbolCount(symTblPtr inSTbl) const
		{
			return ((ll->getSymbolCount(inSTbl) > 0) && (rr->getSymbolCount(inSTbl) > 0)) ? 1 : 0;
		}
		string getIdentifier() const {return "plus"; }
		string str() const { return string("(") + ll->str() + " + " + rr->str() + ")"; }
	private:
		expPtr ll;
		expPtr rr;
};

class Minus : public Expression
{
	public:
		Minus(expPtr inL, expPtr inR) : ll(inL), rr(inR) {}
		double evaluate(symTblPtr inSTbl) const
		{
			return ll->evaluate(inSTbl) - rr->evaluate(inSTbl);
		}
		int getSymbolCount(symTblPtr inSTbl) const
		{
			return ((ll->getSymbolCount(inSTbl) > 0) && (rr->getSymbolCount(inSTbl) > 0)) ? 1 : 0;
		}
		string getIdentifier() const {return "minus"; }
		string str() const { return string("(") + ll->str() + " - " + rr->str() + ")"; }
	private:
		expPtr ll;
		expPtr rr;
};

class Times : public Expression
{
	public:
		Times(expPtr inL, expPtr inR) : ll(inL), rr(inR) {}
		double evaluate(symTblPtr inSTbl) const
		{
			return ll->evaluate(inSTbl) * rr->evaluate(inSTbl);
		}
		int getSymbolCount(symTblPtr inSTbl) const
		{
			return ((ll->getSymbolCount(inSTbl) > 0) && (rr->getSymbolCount(inSTbl) > 0)) ? 1 : 0;
		}
		string getIdentifier() const {return "times"; }
		string str() const { return string("(") + ll->str() + " * " + rr->str() + ")"; }
	private:
		expPtr ll;
		expPtr rr;
};

class Divide : public Expression
{
	public:
		Divide(expPtr inL, expPtr inR) : ll(inL), rr(inR) {}
		double evaluate(symTblPtr inSTbl) const
		{
			return ll->evaluate(inSTbl) / rr->evaluate(inSTbl);
		}
		int getSymbolCount(symTblPtr inSTbl) const
		{
			return ((ll->getSymbolCount(inSTbl) > 0) && (rr->getSymbolCount(inSTbl) > 0)) ? 1 : 0;
		}
		string getIdentifier() const {return "divide"; }
		string str() const { return string("(") + ll->str() + " / " + rr->str() + ")"; }
	private:
		expPtr ll;
		expPtr rr;
};

class Mod : public Expression
{
	public:
		Mod(expPtr inL, expPtr inR) : ll(inL), rr(inR) {}
		double evaluate(symTblPtr inSTbl) const
		{
			return ((int) (ll->evaluate(inSTbl))) % ((int) (rr->evaluate(inSTbl)));
		}
		int getSymbolCount(symTblPtr inSTbl) const
		{
			return ((ll->getSymbolCount(inSTbl) > 0) && (rr->getSymbolCount(inSTbl) > 0)) ? 1 : 0;
		}
		string getIdentifier() const {return "mod"; }
		string str() const { return string("(") + ll->str() + " % " + rr->str() + ")"; }
	private:
		expPtr ll;
		expPtr rr;
};

class Min : public Expression
{
	public:
		Min(expPtr inL, expPtr inR) : ll(inL), rr(inR) {}
		double evaluate(symTblPtr inSTbl) const
		{
			return min(ll->evaluate(inSTbl), rr->evaluate(inSTbl));
		}
		int getSymbolCount(symTblPtr inSTbl) const
		{
			return ((ll->getSymbolCount(inSTbl) > 0) && (rr->getSymbolCount(inSTbl) > 0)) ? 1 : 0;
		}
		string getIdentifier() const {return "min"; }
		string str() const { return string("min(") + ll->str() + ", " + rr->str() + ")"; }
	private:
		expPtr ll;
		expPtr rr;
};

class Max : public Expression
{
	public:
		Max(expPtr inL, expPtr inR) : ll(inL), rr(inR) {}
		double evaluate(symTblPtr inSTbl) const
		{
			return max(ll->evaluate(inSTbl), rr->evaluate(inSTbl));
		}
		int getSymbolCount(symTblPtr inSTbl) const
		{
			return ((ll->getSymbolCount(inSTbl) > 0) && (rr->getSymbolCount(inSTbl) > 0)) ? 1 : 0;
		}
		string getIdentifier() const {return "max"; }
		string str() const { return string("max(") + ll->str() + ", " + rr->str() + ")"; }
	private:
		expPtr ll;
		expPtr rr;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class PipelineManager
{
	public:
		void setDefaultPipeline(pipelinePtr inDefaultPipeline)
		{
			defaultPipeline = inDefaultPipeline;
		}
		void mapSymbols(pipelinePtr inPipeline, set<string> &inSyms)
		{
			for (set<string>::iterator ii = inSyms.begin();
					ii != inSyms.end(); ii++)
			{
				pipeTbl[*ii] = inPipeline;
			}
		}
		void mapSymbol(pipelinePtr inPipeline, string inSym)
		{
			pipeTbl[inSym] = inPipeline;
		}
		bool hasInput(int inSt, string inSym) const
		{
			pipelinePtr pipe;
			if (pipeTbl.find(inSym) != pipeTbl.end())
			{
				pipe = ((PipelineManager*) this)->pipeTbl[inSym];
			}
			else if (!defaultPipeline.isNull())
			{
				pipe = defaultPipeline;
			}
			else
			{
				throw string("No pipe for symbol ") + inSym;
			}

			if (onboardSyms.find(pipe) == onboardSyms.end())
			{
				if (pipe->hasNextSymbol(inSt))
				{
					((PipelineManager*) this)->onboardSyms[pipe] = pipe->getNextSymbol();
				}
			}
			return ((onboardSyms.find(pipe) != onboardSyms.end()) &&
					(((PipelineManager*) this)->onboardSyms[pipe]->getName() == inSym));
		}
		symPtr getNext(string inSym)
		{
			symPtr sym;
			pipelinePtr pipe;
			if (pipeTbl.find(inSym) != pipeTbl.end())
			{
				pipe = ((PipelineManager*) this)->pipeTbl[inSym];
			}
			else if (!defaultPipeline.isNull())
			{
				pipe = defaultPipeline;
			}
			else
			{
				throw string("No pipe for symbol ") + inSym;
			}
			if ((onboardSyms.find(pipe) != onboardSyms.end()) &&
					(onboardSyms[pipe]->getName() == inSym))
			{
				sym = onboardSyms[pipe];
				onboardSyms.erase(pipe);
			}
			return sym;
		}

	private:
		pipelinePtr defaultPipeline;
		map<string, pipelinePtr> pipeTbl;
		map<pipelinePtr, symPtr> onboardSyms;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


#define DBL_MIN -1000
#define DBL_MAX 10000

class Engine
{
	public:
		Engine(StateTable &inStateTbl, PipelineManager &inPipeMgr)
				: stateTbl(inStateTbl), pipeMgr(inPipeMgr) {}

		symPtr generate(symTblPtr ioSymbolTbl)
		{
			while ((ioSymbolTbl->getState() < 0) ||
					(stateTbl.hasNextState(ioSymbolTbl->getState())))
			{
				runTables(stateTbl, ioSymbolTbl, false);
			}

			return ioSymbolTbl->topSym();
		}

////////////////////////////////////////////////////////////////////////////////

		void mapXtTypes(string inSymbol, string inType)
		{
			gXtTypes[inSymbol] = inType;
		}

double getAffinityGoal(symTblPtr inSymbolTbl, string inAffinity)
{
	return gAffinityGoal[inAffinity]->evaluate(inSymbolTbl);
}

void loadTables(StateTable &outStateTbl)
{
	vector<string> calcSyms;

	int state=0;
	char buf[32];

	string curSym="";
	int curSt=-1;
	int rdxCnt=-1;
	int affinCnt=-1;
	string algoArg="";

	for (string line; getline(cin, line); )
	{
		string cmd = trim(line);
		if (((cmd[0] == '-') && (cmd[1] == '-')) || (cmd[0] == '=') || (cmd[0] == ' ')) continue;

		switch (state)
		{
			case 0:
			{
				if (cmd == "SYMBOLS")
				{
					state = 1;
				}
				else if (cmd == "INPUT")
				{
					state = 30;
				}
				else if (cmd == "INVENTORY")
				{
					state = 35;
				}
				else if (cmd == "CALCULATIONS")
				{
					state = 40;
				}
				else if (cmd == "AFFINITIES")
				{
					state = 50;
				}
				break;
			}
			case 1:
			{
				if (cmd == "done")
				{
					state = 0;
				}
				else
				{
					curSym = cmd;
					state = 2;
				}
				break;
			}
			case 2:
			{
				gXtTypes[curSym] = cmd;
				state = 1;
				break;
			}
			case 30:
			{
				if (cmd == "done")
				{
					state = 0;
				}
				else
				{
					curSym = cmd;
					state = 31;
				}
				break;
			}
			case 31:
			{
				if (cmd == "done")
				{
					state = 30;
				}
				else
				{
					gInputTbl[curSym].push_back(atoi(cmd.c_str()));
				}
				break;
			}
			case 35:
			{
				if (cmd == "done")
				{
					state = 0;
				}
				else
				{
					curSym = cmd;
					state = 36;
				}
				break;
			}
			case 36:
			{
				if (cmd == "done")
				{
					state = 35;
				}
				else
				{
					gInventoryTbl[curSym].push_back(atoi(cmd.c_str()));
				}
				break;
			}
			case 40:
			{
				if (cmd == "done")
				{
					state = 0;
				}
				else
				{
					curSym = cmd;
					state = 41;
				}
				break;
			}
			case 41:
			{
				if (cmd == "done")
				{
					gComputeTbl[curSym] = compileEvaluation(calcSyms);
					calcSyms.clear();
					state = 40;
				}
				else
				{
					calcSyms.push_back(cmd);
				}
				break;
			}
			case 50:
			{
				curSym = cmd;
				state = 51;
				break;
			}
			case 51:
			{
				affinCnt = atoi(cmd.c_str());
				if (affinCnt < 1) throw string("Affinity control count must be greater than 0.  It was '") + cmd + "'";
				state = 52;
				break;
			}
			case 52:
			{
				gAffinityCtl[cmd] = curSym;
				affinCnt--;
				if (affinCnt < 1) state = 56;
				break;
			}
			case 53:
			{
				if (cmd == "done")
				{
					state = 0;
				}
				else if (cmd == "hint")
				{
					state = 54;
				}
				else if (cmd == "goal")
				{
					state = 55;
				}
				else
				{
					algoArg = cmd;
					state = 57;
				}
				break;
			}
			case 54:
			{
				if ((cmd == "done") || (cmd == "goal"))
				{
					gAffinityHint[curSym] = compileEvaluation(calcSyms);
					calcSyms.clear();
					if (cmd == "goal")
					{
						state = 55;
					}
					else
					{
						state = 0;
					}
				}
				else
				{
					calcSyms.push_back(cmd);
				}
				break;
			}
			case 55:
			{
				if ((cmd == "done") || (cmd == "hint"))
				{
					gAffinityGoal[curSym] = compileEvaluation(calcSyms);
					calcSyms.clear();
					if (cmd == "hint")
					{
						state = 54;
					}
					else
					{
						state = 0;
					}
				}
				else
				{
					calcSyms.push_back(cmd);
				}
				break;
			}
			case 56:
			{
				gAffinityAlgorithm[curSym] = cmd;
				state = 53;
				break;
			}
			case 57:
			{
				gAffinityAlgoArg[curSym][algoArg] = cmd;
				state = 53;
				break;
			}
		}
	}
}

	private:

expPtr compileEvaluation(vector<string> &inCalcSyms)
{
	expPtr ret;

	stack<expPtr> evStk;

	for (int ii=inCalcSyms.size()-1; ii>=0; ii--)
	{
		if (inCalcSyms[ii] == "plus")
		{
			expPtr ll = evStk.top();
			evStk.pop();
			expPtr rr = evStk.top();
			evStk.pop();
			evStk.push(new Plus(ll, rr));
		}
		else if (inCalcSyms[ii] == "minus")
		{
			expPtr ll = evStk.top();
			evStk.pop();
			expPtr rr = evStk.top();
			evStk.pop();
			evStk.push(new Minus(ll, rr));
		}
		else if (inCalcSyms[ii] == "times")
		{
			expPtr ll = evStk.top();
			evStk.pop();
			expPtr rr = evStk.top();
			evStk.pop();
			evStk.push(new Times(ll, rr));
		}
		else if (inCalcSyms[ii] == "divide")
		{
			expPtr ll = evStk.top();
			evStk.pop();
			expPtr rr = evStk.top();
			evStk.pop();
			evStk.push(new Divide(ll, rr));
		}
		else if (inCalcSyms[ii] == "mod")
		{
			expPtr ll = evStk.top();
			evStk.pop();
			expPtr rr = evStk.top();
			evStk.pop();
			evStk.push(new Mod(ll, rr));
		}
		else if (inCalcSyms[ii] == "min")
		{
			expPtr ll = evStk.top();
			evStk.pop();
			expPtr rr = evStk.top();
			evStk.pop();
			evStk.push(new Min(ll, rr));
		}
		else if (inCalcSyms[ii] == "max")
		{
			expPtr ll = evStk.top();
			evStk.pop();
			expPtr rr = evStk.top();
			evStk.pop();
			evStk.push(new Max(ll, rr));
		}
		else if (inCalcSyms[ii] == "index")
		{
			expPtr ll = evStk.top();
			evStk.pop();
			expPtr rr = evStk.top();
			evStk.pop();
			evStk.push(new Variable(rr->getIdentifier(), ll));
		}
		else if (inCalcSyms[ii] == "path")
		{
			expPtr cc = evStk.top();
			evStk.pop();
			symTblPtr ss;
			int cnt = cc->evaluate(ss);
			Path *pth = new Path();
			for (int ii=0; ii<cnt; ii++)
			{
				expPtr ee = evStk.top();
				evStk.pop();
				pth->add(ee);
			}
			evStk.push(pth);
		}
		else if (inCalcSyms[ii] == "sum")
		{
			expPtr aa = evStk.top();
			evStk.pop();
			evStk.push(new Sum(aa));
		}
		else if (inCalcSyms[ii] == "count")
		{
			expPtr aa = evStk.top();
			evStk.pop();
			evStk.push(new Count(aa));
		}
		else if (inCalcSyms[ii] == "each")
		{
			expPtr col = evStk.top();
			evStk.pop();
			expPtr var = evStk.top();
			evStk.pop();
			evStk.push(new Each(col, var));
		}
		else if (((inCalcSyms[ii][0] >= '0') && (inCalcSyms[ii][0] <= '9')) ||
				(inCalcSyms[ii][0] == '-'))
		{
			evStk.push(new Value(atoi(inCalcSyms[ii].c_str())));
		}
		else
		{
			evStk.push(new Variable(inCalcSyms[ii]));
		}
	}
	if (evStk.size() == 1)
	{
		ret = evStk.top();
		evStk.pop();
	}
	else
	{
		char buf[64];
		sprintf(buf, "Bad expression, stk=%d", evStk.size());
		throw string(buf);
	}

	return ret;
}

////////////////////////////////////////////////////////////////////////////////
// Parameters for the engine

		map<string, string> gXtTypes;

		map<string, vector<int> > gInputTbl;
		map<string, vector<int> > gInventoryTbl;
		map<string, expPtr> gComputeTbl;

		map<string, string> gAffinityCtl;
		map<string, expPtr> gAffinityHint;
		map<string, expPtr> gAffinityGoal;
		map<string, string> gAffinityAlgorithm;
		map<string, map<string, string> > gAffinityAlgoArg;

		PipelineManager &pipeMgr;
		StateTable &stateTbl;

////////////////////////////////////////////////////////////////////////////////

		map<string, vector<int> > gResolutions;
		map<string, int> gResolutionPts;

		double doResolve(string inSym, StateTable &inStateTbl, symTblPtr inSTbl)
		{
			if ((gResolutions.find(inSym) == gResolutions.end()) ||
				(gResolutions[inSym].size() <= gResolutionPts[inSym]))
			{
		//cout << "RESOLVING..." << endl;

				string sym = inSym;

				if (gAffinityCtl.find(sym) == gAffinityCtl.end())
					throw string("Can't find any affinity for ") + sym;

				string mstAffinity = gAffinityCtl[sym];

				// I would in theory do something with this - but only have BFS so far.
				string algo = gAffinityAlgorithm[mstAffinity];

				int openQ = atoi(gAffinityAlgoArg[mstAffinity]["openQ"].c_str());
				int avgQ = atoi(gAffinityAlgoArg[mstAffinity]["avgQ"].c_str());
				int maxDev = atoi(gAffinityAlgoArg[mstAffinity]["maxDev"].c_str());

		//cout << "Algorithm: " << algo << ", openQ:" << openQ << ", avgQ:" << avgQ << ", maxDev:" << maxDev << endl;


				double maxAfVal = DBL_MIN;
				double rngAvgTot = 0, rngMin=DBL_MAX, rngMax=DBL_MIN;
				int rngAvgCnt = 0;

				map<string, queue<pair<double, pair<symTblPtr, vector<int> > > > > pq;

				for (int ii=0; ii<gInventoryTbl[sym].size(); ii++)
				{
					vector<int> vct;
					vct.push_back(ii);
					symTblPtr stp = inSTbl->fork();
					pq[sym].push(make_pair(DBL_MIN, make_pair(stp, vct)));
					rngAvgTot += DBL_MIN;
					rngAvgCnt++;
				}

				while (! pq[sym].empty())
				{
					if (gAffinityCtl.find(sym) == gAffinityCtl.end())
						throw string("Can't find any affinity for ") + sym;

					string affinity = gAffinityCtl[sym];

					symTblPtr sTbl = pq[sym].front().second.first;
					vector<int> vct = pq[sym].front().second.second;
					double aff0 = pq[sym].back().first;
					pq[sym].pop();

					rngAvgTot -= aff0;
					rngAvgCnt--;
					if (rngMax < aff0)
					{
						rngMax = aff0;
					}

					int preSt = sTbl->getState();
					symPtr sp = new Symbol(sym, gInventoryTbl[sym][vct.back()]);
					int nxt = inStateTbl.getNextState(preSt, sym);
					sTbl->push(nxt, sp);

					sym = "";
					while ((sym == "") && (inStateTbl.hasNextState(sTbl->getState())))
					{
						sym = runTables(inStateTbl, sTbl, true);
					}

					double afVal = 0;
					if (gAffinityGoal[affinity]->getSymbolCount(sTbl) > 0)
					{
						afVal = gAffinityGoal[affinity]->evaluate(sTbl);
					}
					else
					{
						afVal = gAffinityHint[affinity]->evaluate(sTbl);
					}

					if (sym == "")
					{
						if (maxAfVal < afVal)
						{
							maxAfVal = afVal;
							gResolutions[inSym] = vct;
							gResolutionPts[inSym] = 0;
						}
						sym = inSym;
					}
					else
					{
						if ((pq[sym].size() < openQ) ||
							((pq[sym].size() < avgQ) && (afVal > (rngAvgTot / rngAvgCnt))) ||
							(afVal >= (rngMax - maxDev)))
						{
							for (int ii=0; ii<gInventoryTbl[sym].size(); ii++)
							{
								vector<int> newVct = vct;
								newVct.push_back(ii);
								pq[sym].push(make_pair(afVal, make_pair(sTbl->fork(), newVct)));
								rngAvgTot += afVal;
								rngAvgCnt++;
							}
						}
					}
				}
			}

			int resol = gResolutions[inSym][gResolutionPts[inSym]];
			gResolutionPts[inSym]++;

			return gInventoryTbl[inSym][resol];
		}

////////////////////////////////////////////////////////////////////////////////

		void reduce(string inSym, symPtr ioSp, StateTable &inStateTbl, symTblPtr ioSTbl)
		{
			int scnt = inStateTbl.getRdxFrameCount(ioSTbl->getState());

			string arrSym="";
			if (! inStateTbl.isRdxFrame(ioSTbl->getState()))
			{
				arrSym = inStateTbl.getRdxSubSym(ioSTbl->getState());
	//arrSym = ioSTbl->topSym()->getName();
			}

			for (int jj=0; ((jj<scnt) ||
					((!inStateTbl.isRdxFrame(ioSTbl->getState())) &&
					(ioSTbl->stackSize() > 0) &&
					(ioSTbl->topSym()->getName() == arrSym))); jj++)
			{

				ioSp->addSym(ioSTbl->topSym());

				ioSTbl->pop();
			}
		}

////////////////////////////////////////////////////////////////////////////////

		string runTables(StateTable &inStateTbl, symTblPtr ioSTbl, bool inHypothesis)
		{
			string needResolve = "";

		//cout << "run state: " << ioSTbl->getState() << ", hypothesis? " << inHypothesis << endl;
			int nxt=-1;
			string sym;
			symPtr sp;

			if (inStateTbl.isRdxState(ioSTbl->getState()))
			{
				int lacnt = inStateTbl.getRdxLACount(ioSTbl->getState());
				ioSTbl->setAside(lacnt);

				sym = inStateTbl.getRdxSym(ioSTbl->getState());

		//cout << "\tRDX: [" << sym << "]" << endl;

				sp = new Symbol(sym);
				reduce(sym, sp, inStateTbl, ioSTbl);
				nxt = inStateTbl.getNextState(ioSTbl->getState(), sym);
			}
			else
			{
				for (vector<string>::iterator jj = inStateTbl.tokenBegin(ioSTbl->getState());
						((jj != inStateTbl.tokenEnd(ioSTbl->getState())) && (nxt < 0)); jj++)
				{
					sym = *jj;
					nxt = inStateTbl.getNextState(ioSTbl->getState(), sym);

					if (gXtTypes[sym] == "compute")
					{
						expPtr ev = gComputeTbl[sym];
						double vv = ev->evaluate(ioSTbl);

						sp = new Symbol(sym, vv);
					}
					else if (gXtTypes[sym] == "resolve")
					{
						if (!inHypothesis)
						{
							double vv = doResolve(sym, inStateTbl, ioSTbl);
							sp = new Symbol(sym, vv);
						}
						else
						{
							needResolve = sym;
						}
					}
					else if (gXtTypes[sym] == "distribution")
					{
						string kk = sym + "_iter";
						if (!ioSTbl->hasKV(kk))
						{
							ioSTbl->setKV(kk, 0);
						}
						int ii = ioSTbl->getKV(kk);
						ioSTbl->setKV(kk, ii+1);
						if (ii < gInputTbl[sym].size())
						{
							int vv = gInputTbl[sym][ii];
							sp = new Symbol(sym, vv);
						}
						else
						{
							nxt = -1;
						}
					}
					else if (gXtTypes[sym] == "pipe")
					{
						if (pipeMgr.hasInput(ioSTbl->getState(), sym))
						{
							sp = pipeMgr.getNext(sym);
						}
						else
						{
							nxt = -1;
						}
					}
					else
					{
						// A compound symbol...
						nxt = -1;
					}
				}
				if (nxt < 0)
				{
					char buf[64];
					sprintf(buf, "No usable transitions in state %d", ioSTbl->getState());
					throw string(buf);
				}
			}

			if (! sp.isNull())
			{
				ioSTbl->push(nxt, sp);
				// In case there was an rdx with a look ahead buffer, put the L/As back.

				while (ioSTbl->hasAside())
				{
					symPtr laSym = ioSTbl->popNextAside();
					nxt = inStateTbl.getNextState(ioSTbl->getState(), laSym->getName());
					ioSTbl->push(nxt, laSym);
				}
			}

			return needResolve;
		}
};

#endif
