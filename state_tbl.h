#ifndef _STATE_TBL_H_
#define _STATE_TBL_H_

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

using namespace std;

////////////////////////////////////////////////////////////////////////////////

class StateTable
{
	public:
		StateTable(	string inRoot, map<string, vector<vector<string> > > &inRules,
					map<string, map<int, map<int, pair<char, pair<int, int> > > > > &inModifiers)
		{
			loadStateTable(inRoot, inRules, inModifiers);
		}

		StateTable(symPtr inCfg)
		{
			map<string, vector<vector<string> > > rules;
			map<string, map<int, map<int, pair<char, pair<int, int> > > > > modifiers;
			map<string, map<string, string> > outAliases;

			string root = StateTable::cvtSymbol2Struct(inCfg, rules, modifiers, outAliases);
			loadStateTable(root, rules, modifiers);
		}

		static string cvtSymbol2Struct(symPtr inCfg,
					map<string, vector<vector<string> > > &outRules,
					map<string, map<int, map<int, pair<char, pair<int, int> > > > > &outModifiers,
					map<string, map<string, string> > &outAliases);

		static string cvtSymbolDef(symPtr inSymDef, string inTgt, int inLn, int inPt,
			map<string, map<string, string> > &outAliases);

		static string makeAlias(string inSym, int inLine, int inPt);

		void dumpRdx(	pair<string, pair<int, int> > &inRdxCmd,
						int inTab)
		{
			string sym = inRdxCmd.first;
			int sz = inRdxCmd.second.first;
			int la = inRdxCmd.second.second;

			for (int tt=0; tt<inTab; tt++) cout << "\t";
			cout << "RDX: [" << sym << "] count: " << sz << ", look-ahead: " << la << endl;
		}

		void dumpState(	set<pair<string, pair<int, int> > > &ioPts,
						map<string, vector<vector<string> > > &inRules,
						map<string, map<int, map<int, pair<char, pair<int, int> > > > > &inModifiers,
						int inTab)
		{
			for (set<pair<string, pair<int, int> > >::iterator ii = ioPts.begin();
					 ii != ioPts.end(); ii++)
			{
				string sym = ii->first;
				int ln = ii->second.first;
				int pt = ii->second.second;

				for (int tt=0; tt<inTab; tt++) cout << "\t";
				cout << sym << " ->";

				for (int jj=0; jj<inRules[sym][ln].size(); jj++)
				{
					string subSym = inRules[sym][ln][jj];
					if (jj == pt)
					{
						cout << " .";
					}
					cout << " " << subSym;
					if ((inModifiers.find(sym) != inModifiers.end()) &&
						(inModifiers[sym].find(ln) != inModifiers[sym].end()) &&
						(inModifiers[sym][ln].find(jj) != inModifiers[sym][ln].end()))
					{
						cout << inModifiers[sym][ln][jj].first;
					}
				}

				if (pt == inRules[sym][ln].size())
				{
					cout << " .";
				}

				cout << ";" << endl;
			}
		}

		int getRootState() { return rootState; }
		vector<string>::iterator symBegin(int inState) const
		{
			return ((StateTable*) this)->xtSymTbl[inState].begin();
		}
		vector<string>::iterator symEnd(int inState) const
		{
			return ((StateTable*) this)->xtSymTbl[inState].end();
		}
		vector<string>::iterator tokenBegin(int inState) const
		{
			return ((StateTable*) this)->tokens[inState].begin();
		}
		vector<string>::iterator tokenEnd(int inState) const
		{
			return ((StateTable*) this)->tokens[inState].end();
		}
		int getNextState(int inState, string inSym) const
		{
			return ((StateTable*) this)->xtTbl[inState][inSym];
		}
		bool hasNextState(int inState) const
		{

			bool ret = (((((StateTable*) this)->xtTbl.find(inState) != xtTbl.end()) &&
					(((StateTable*) this)->xtTbl[inState].size() > 0)) ||
					(rdxTbl.find(inState) != rdxTbl.end()));
			return ret;
		}

		void getAllStates(vector<int> &outStates) const
		{
			for (map<int, vector<string> >::iterator ii = ((StateTable*) this)->xtSymTbl.begin();
					ii != xtSymTbl.end(); ii++)
			{
				outStates.push_back(ii->first);
			}
		}

		bool isRdxState(int inState) const
		{
			return (rdxTbl.find(inState) != rdxTbl.end());
		}
		string getRdxSym(int inState) const
		{
			if (rdxTbl.find(inState) == rdxTbl.end()) throw string("No RDX: getRdxSym()");
			return ((StateTable*) this)->rdxTbl[inState].first;
		}
		bool isRdxFrame(int inState) const
		{
			return ((rdxTbl.find(inState) != rdxTbl.end()) &&
					(((StateTable*) this)->rdxTbl[inState].second.first > 0));
		}
		string getRdxSubSym(int inState) const
		{
			if (rdxTbl.find(inState) == rdxTbl.end()) throw string("No RDX: getRdxSubSym()");
			return ((StateTable*) this)->rdxSubSymbols[inState];
		}
		int getRdxFrameCount(int inState) const
		{
			if (rdxTbl.find(inState) == rdxTbl.end()) throw string("No RDX: getRdxFrameCount()");
			return ((StateTable*) this)->rdxTbl[inState].second.first;
		}
		int getRdxLACount(int inState) const
		{
			if (rdxTbl.find(inState) == rdxTbl.end()) throw string("No RDX: getRdxLACount()");
			return ((StateTable*) this)->rdxTbl[inState].second.second;
		}

		void dump(int inTab)
		{
			for (int tt=0; tt<inTab; tt++) cout << "\t";
			cout << "STATE TBL" << endl;
			for (int tt=0; tt<inTab; tt++) cout << "\t";
			cout << "{" << endl;

			for (int st=0; true; st++)
			{
				if (xtSymTbl.find(st) != xtSymTbl.end())
				{
					for (int tt=0; tt<=inTab; tt++) cout << "\t";
					cout << st << ":" << endl;
					for (map<string, int>::iterator ii=xtTbl[st].begin(); ii != xtTbl[st].end(); ii++)
					{
						for (int tt=0; tt<=(inTab+1); tt++) cout << "\t";
						cout << ii->first << " -> " << ii->second << endl;
					}
				}
				else if (rdxTbl.find(st) != rdxTbl.end())
				{
					for (int tt=0; tt<=inTab; tt++) cout << "\t";
					cout << st << ": RDX [" << rdxTbl[st].first << "], frm: " << rdxTbl[st].second.first << ", LA: " << rdxTbl[st].second.second << ", subSym: [" << rdxSubSymbols[st] << "]" << endl;
				}
				else
				{
					break;
				}
			}

			for (int tt=0; tt<inTab; tt++) cout << "\t";
			cout << "}" << endl;
		}

	private:
		int rootState;
		map<int, vector<string> > xtSymTbl;
		map<int, map<string, int> > xtTbl;
		map<int, pair<string, pair<int, int> > > rdxTbl;
		map<int, string> rdxSubSymbols;
		map<int, vector<string> > tokens;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

		void loadStateTable(	string inRoot, map<string, vector<vector<string> > > &inRules,
					map<string, map<int, map<int, pair<char, pair<int, int> > > > > &inModifiers)
		{
			set<pair<string, pair<int, int> > > stv;
			for (int ii=0; ii<inRules[inRoot].size(); ii++)
			{
				stv.insert(make_pair(inRoot, make_pair(ii, 0)));
			}
			fillStates(stv, inRules, inModifiers);

			map<set<pair<string, pair<int, int> > >, int> stMp;
			map<pair<string, pair<int, int> >, int> rdxMp;

			map<string, set<string> > las;

			rootState = compile(stv, stMp, rdxMp, las, inRules, inModifiers);

			xtSymTbl[rootState].push_back(inRoot);
			xtTbl[rootState][inRoot] = 99999;
		}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

		void fillStates(set<pair<string, pair<int, int> > > &ioPts,
						map<string, vector<vector<string> > > &inRules,
						map<string, map<int, map<int, pair<char, pair<int, int> > > > > &inModifiers)
		{
			bool didAdd=false;
			for (set<pair<string, pair<int, int> > >::iterator ii = ioPts.begin();
					ii != ioPts.end(); ii++)
			{
				string sym = ii->first;
				int ln = ii->second.first;
				int pt = ii->second.second;

				if (pt < inRules[sym][ln].size())
				{
					string nxtSym = inRules[sym][ln][pt];

					if ((inModifiers.find(sym) != inModifiers.end()) &&
						(inModifiers[sym].find(ln) != inModifiers[sym].end()) &&
						(inModifiers[sym][ln].find(pt) != inModifiers[sym][ln].end()) &&
						(inModifiers[sym][ln][pt].first == '*'))
					{
						pair<string, pair<int, int> > pp = make_pair(sym, make_pair(ln, pt+1));
						if (ioPts.find(pp) == ioPts.end())
						{
							ioPts.insert(pp);
							didAdd = true;
						}
					}

					if (inRules.find(nxtSym) != inRules.end())
					{
						for (int kk=0; kk<inRules[nxtSym].size(); kk++)
						{
							pair<string, pair<int, int> > pp = make_pair(nxtSym, make_pair(kk, 0));
							if (ioPts.find(pp) == ioPts.end())
							{
								ioPts.insert(pp);
								didAdd = true;
							}
						}
					}
				}
				else
				{
					for (set<pair<string, pair<int, int> > >::iterator jj = ioPts.begin();
							jj != ioPts.end(); jj++)
					{
						string sym2 = jj->first;
						int ln2 = jj->second.first;
						int pt2 = jj->second.second;

						if (pt2 < inRules[sym2][ln2].size())
						{
							string subSym = inRules[sym2][ln2][pt2];

							if (sym == subSym)
							{
								pair<string, pair<int, int> > pp = make_pair(sym2, make_pair(ln2, pt2+1));
								if (ioPts.find(pp) == ioPts.end())
								{
									ioPts.insert(pp);
									didAdd = true;
								}
							}
						}
					}
				}
			}
			if (didAdd)
			{
				fillStates(ioPts, inRules, inModifiers);
			}
		}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

		bool expandToTokens(set<string> &outExp,
						string inSym,
						map<string, vector<vector<string> > > &inRules,
						map<string, map<int, map<int, pair<char, pair<int, int> > > > > &inModifiers,
						set<string> &ioVisit)
		{
			bool ranPastEnd = false;

			if (ioVisit.find(inSym) == ioVisit.end())
			{
				ioVisit.insert(inSym);

				if (inRules.find(inSym) == inRules.end())
				{
					outExp.insert(inSym);
				}
				else
				{
					for (int ln=0; ln<inRules[inSym].size(); ln++)
					{
						int pt = 0;
						bool anyRanPast=false;
						while ((pt < inRules[inSym][ln].size()) &&
							((pt == 0) ||
							(inModifiers.find(inSym) != inModifiers.end()) &&
							(inModifiers[inSym].find(ln) != inModifiers[inSym].end()) &&
							(inModifiers[inSym][ln].find(pt-1) != inModifiers[inSym][ln].end()) &&
							(inModifiers[inSym][ln][pt-1].first == '*')))
						{
							string fSym = inRules[inSym][ln][pt];
							if (inRules.find(fSym) == inRules.end())
							{
								outExp.insert(fSym);
							}
							else
							{
								anyRanPast |= expandToTokens(outExp, fSym, inRules, inModifiers, ioVisit);
							}
							pt++;
						}
						if ((pt == inRules[inSym][ln].size()) &&
							(((inModifiers.find(inSym) != inModifiers.end()) &&
							(inModifiers[inSym].find(ln) != inModifiers[inSym].end()) &&
							(inModifiers[inSym][ln].find(pt-1) != inModifiers[inSym][ln].end()) &&
							(inModifiers[inSym][ln][pt-1].first == '*')) ||
							(anyRanPast)))
						{
							ranPastEnd = true;
						}
					}
				}
			}

			return ranPastEnd;
		}

		void fillTrailingTokens(
						string inXt,
						set<string> &inProds,
						map<string, set<string> > &outNextLAs,
						map<string, set<string> > &inParentLAs,
						map<string, set<string> > &inNextXtLAs,
						map<string, set<string> > &inLAs,
						map<string, vector<vector<string> > > &inRules,
						map<string, map<int, map<int, pair<char, pair<int, int> > > > > &inModifiers,
						set<string> &ioVisit)
		{
			if (ioVisit.find(inXt) == ioVisit.end())
			{
//cout << "FILL [" << inXt << "]" << endl;

				ioVisit.insert(inXt);
				for (set<string>::iterator ii = inProds.begin();
						ii != inProds.end(); ii++)
				{
					string prod = *ii;
					if (inParentLAs.find(prod) != inParentLAs.end())
					{
						fillTrailingTokens(prod, inParentLAs[prod], outNextLAs, inParentLAs, inNextXtLAs, inLAs, inRules, inModifiers, ioVisit);
						for (set<string>::iterator jj = outNextLAs[prod].begin();
								jj != outNextLAs[prod].end(); jj++)
						{
							outNextLAs[inXt].insert(*jj);
						}
					}
					if (inNextXtLAs.find(prod) != inNextXtLAs.end())
					{
						set<string> exp;
						set<string> visit;
						for (set<string>::iterator jj = inNextXtLAs[prod].begin();
								jj != inNextXtLAs[prod].end(); jj++)
						{
							outNextLAs[inXt].insert(*jj);
						}
					}
					else if ((inParentLAs.find(prod) == inParentLAs.end()) &&
						(inLAs.find(prod) != inLAs.end()))
					{
						for (set<string>::iterator jj = inLAs[prod].begin();
								jj != inLAs[prod].end(); jj++)
						{
							outNextLAs[inXt].insert(*jj);
						}
					}
				}
			}
		}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//	1. Loop through each rule points (inPts)
//		1.1 Gather all next possible XTs from the given set of rule points
//			1.1.1 Gather the new rule points under the XT
//			1.1.2 Gather all trailing tokens that follow these XTs
//		1.2 Gather all rules at their end - reductions
//	2. For each 'next to end of rule' transition, determine which tokens follow it.
//	3. For each 'end of rule': create an reduction state with L/A using
//			the rdx symbol's trailing tokens as the transition.
//	4. Loop through each XT
//		4.1 If XT's new rule is at its end and has no conflicting shifts/rdxs
//				then create a reduction state without L/A
//		4.2 Otherwise shift to recursion created state.  (Add trailing tokens)
////////////////////////////////////////////////////////////////////////////////

		int compile(	set<pair<string, pair<int, int> > > &inPts,
						map<set<pair<string, pair<int, int> > >, int> &ioStMp,
						map<pair<string, pair<int, int> >, int> &ioRdxMp,
						map<string, set<string> > &inLAs,
						map<string, vector<vector<string> > > &inRules,
						map<string, map<int, map<int, pair<char, pair<int, int> > > > > &inModifiers)
		{
			if (ioStMp.find(inPts) == ioStMp.end())
			{
// Create this state...
				int myState = ioStMp.size() + ioRdxMp.size();
				ioStMp[inPts] = myState;

				char myStateStr[16];
				sprintf(myStateStr, "%d", myState);

#ifdef _DEBUG_

cout << "--------------------------------------------------------------------------------" << endl;
cout << "\tState " << myState << ":" << endl;
dumpState(inPts, inRules, inModifiers, 2);

for (map<string, set<string> >::iterator xx = inLAs.begin();
		 xx != inLAs.end(); xx++)
{
	cout << "\t\tLA " << xx->first << ": (";
	string ss="";
	for (set<string>::iterator yy = xx->second.begin();
			 yy != xx->second.end(); yy++)
	{
		if (ss.length() > 0) ss += ", ";
		ss += *yy;
	}
	cout << ss << ")" << endl;
}
cout << "--------------------------------------------------------------------------------" << endl;

#endif

				map<string, set<pair<string, pair<int, int> > > > nextXts;
				map<string, set<string> > nextXtLAs;
				map<string, set<string> > parentLAs;
				map<string, set<pair<string, pair<int, int> > > > rdxs;
// 1. Loop through each rule points (inPts)
				for (set<pair<string, pair<int, int> > >::iterator ii = inPts.begin();
						ii != inPts.end(); ii++)
				{
					string prod = ii->first;
					int ln = ii->second.first;
					int pt = ii->second.second;
// 1.1 Gather all next possible XTs from the given set of rule points
					if (pt < inRules[prod][ln].size())
					{
						string xt = inRules[prod][ln][pt];
// 1.1.1 Gather the new rule points under the XT
						nextXts[xt].insert(make_pair(prod, make_pair(ln, pt+1)));

						if ((inModifiers.find(prod) != inModifiers.end()) &&
							(inModifiers[prod].find(ln) != inModifiers[prod].end()) &&
							(inModifiers[prod][ln].find(pt) != inModifiers[prod][ln].end()))
						{
							// If we're at the last symbol in the frame but it's a zero plus series then add to reductions.
							if ((pt == (inRules[prod][ln].size() - 1)) &&
								(inModifiers[prod][ln][pt].first == '*'))
							{
								rdxs[prod].insert(make_pair(prod, make_pair(ln, pt+1)));
							}
							// Since the XT is a series, XT takes us back to where we were.
							nextXts[xt].insert(make_pair(prod, make_pair(ln, pt)));
						}

// 1.1.2 Gather all trailing tokens that follow these XTs
						if (pt == (inRules[prod][ln].size() - 1))
						{
							parentLAs[xt].insert(prod);
						}
						else
						{
							set<string> visit;
							bool ranPast = expandToTokens(nextXtLAs[xt],
											inRules[prod][ln][pt+1],
											inRules, inModifiers,
											visit);
							if (ranPast)
							{
								parentLAs[xt].insert(prod);
							}
						}
					}
// 1.2 Gather all rules at their end - reductions
					else
					{
						rdxs[prod].insert(*ii);
					}
				}

// 2. For each 'next to end of rule' transition, determine which tokens follow it.
				map<string, set<string> > nextLAs;
				for (map<string, set<string> >::iterator ii = parentLAs.begin();
						ii != parentLAs.end(); ii++)
				{
					set<string> visit;
					fillTrailingTokens(ii->first, ii->second, nextLAs, parentLAs, nextXtLAs, inLAs, inRules, inModifiers, visit);

#ifdef _DEBUG_

cout <<  "NEXTLA[st:" << myState << "]: " << ii->first << ": (";
string ss="";
for (set<string>::iterator jj = nextLAs[ii->first].begin();
		jj != nextLAs[ii->first].end(); jj++)
{
	if (ss.length() > 0) ss += ", ";
	ss += *jj;
}
cout << ss + ")" << endl;
#endif
				}
				for (map<string, set<string> >::iterator ii = nextXtLAs.begin();
						ii != nextXtLAs.end(); ii++)
				{

#ifdef _DEBUG_

cout <<  "XT LA[st:" << myState << "]: " << ii->first << ": (";

#endif

					string ss;
					for (set<string>::iterator jj = nextXtLAs[ii->first].begin();
							jj != nextXtLAs[ii->first].end(); jj++)
					{
						if (ss.length() > 0) ss += ", ";
						ss += *jj;
						nextLAs[ii->first].insert(*jj);
					}

#ifdef _DEBUG_

cout << ss + ")" << endl;

#endif
				}
				for (set<pair<string, pair<int, int> > >::iterator ii = inPts.begin();
						ii != inPts.end(); ii++)
				{
					string prod = ii->first;
					for (set<string>::iterator jj = inLAs[prod].begin();
							jj != inLAs[prod].end(); jj++)
					{
						nextLAs[prod].insert(*jj);
					}
				}

// 3. For each 'end of rule' state: create a reduction state with L/A using the rdx symbol's trailing tokens as the transition.

				for (map<string, set<pair<string, pair<int, int> > > >::iterator ii = rdxs.begin();
						ii != rdxs.end(); ii++)
				{
					string prod = ii->first;
					string seriesMbr = "";
					int frmLen = 0;
					for (set<pair<string, pair<int, int> > >::iterator jj = ii->second.begin();
							jj != ii->second.end(); jj++)
					{
						int ln = jj->second.first;
						int pt = jj->second.second;
						if ((inModifiers.find(prod) != inModifiers.end()) &&
							(inModifiers[prod].find(ln) != inModifiers[prod].end()) &&
							(inModifiers[prod][ln].find(pt-1) != inModifiers[prod][ln].end()))
						{
							if ((frmLen > 0) ||
									((seriesMbr.length() > 0) &&
									(seriesMbr != inRules[prod][ln][pt-1])))
							{
								throw string("Redundant reduction0: prod: ") + prod;
							}
							seriesMbr = inRules[prod][ln][pt-1];
						}
						else
						{
							if (((frmLen > 0) && (frmLen != pt)) || (seriesMbr.length() > 0))
							{
								throw string("Redundant reduction1: prod: ") + prod;
							}
							frmLen = pt;
						}
					}
					pair<string, pair<int, int> > rdxCmd(prod, make_pair(frmLen, 1));
					if (ioRdxMp.find(rdxCmd) == ioRdxMp.end())
					{
						int rdxSt = ioStMp.size() + ioRdxMp.size();
						ioRdxMp[rdxCmd] = rdxSt;

#ifdef _DEBUG_

cout << "--------------------------------------------------------------------------------" << endl;
if ((inLAs.find(prod) != inLAs.end()) &&
						(inLAs[prod].size()> 0))
{
	cout << "LAs for " << prod << endl;
	for (set<string>::iterator jj = inLAs[prod].begin();
			jj != inLAs[prod].end(); jj++)
	{
		string disambig = *jj;
		cout << "ST:" << myState << " -[" << disambig << "]->" << endl;
	}
}
/*
else if (nextXtLAs.find(prod) != nextXtLAs.end())
{
	cout << "Nexts for " << prod << endl;
	for (set<string>::iterator jj = nextXtLAs[prod].begin();
			jj != nextXtLAs[prod].end(); jj++)
	{
		string disambig = *jj;
		cout << "ST:" << myState << " -[" << disambig << "]->" << endl;
	}
}
else if (nextLAs.find(prod) != nextLAs.end())
{
	cout << "Nexts for " << prod << endl;
	for (set<string>::iterator jj = nextLAs[prod].begin();
			jj != nextLAs[prod].end(); jj++)
	{
		string disambig = *jj;
		cout << "ST:" << myState << " -[" << disambig << "]->" << endl;
	}
}
*/
cout << "\tState " << rdxSt << ":" << endl;
dumpRdx(rdxCmd, 2);
cout << "--------------------------------------------------------------------------------" << endl;

#endif

					}
					if ((inLAs.find(prod) != inLAs.end()) &&
						(inLAs[prod].size() > 0))
					{
						for (set<string>::iterator jj = inLAs[prod].begin();
								jj != inLAs[prod].end(); jj++)
						{
							string disambig = *jj;
							if (xtTbl[myState].find(disambig) != xtTbl[myState].end())
							{
								throw string("Conflict1 in look ahead for reduction of ") + prod + " using XT " + disambig + ", in state " + myStateStr;
							}
							xtSymTbl[myState].push_back(disambig);
							xtTbl[myState][disambig] = ioRdxMp[rdxCmd];
							if (inRules.find(disambig) == inRules.end())
							{
								tokens[myState].push_back(disambig);
							}
							rdxTbl[ioRdxMp[rdxCmd]] = rdxCmd;
							rdxSubSymbols[ioRdxMp[rdxCmd]] = seriesMbr;
						}
					}
/*
					else if ((nextXtLAs.find(prod) != nextXtLAs.end()) &&
							(nextXtLAs[prod].size() > 0))
					{
						for (set<string>::iterator jj = nextXtLAs[prod].begin();
								jj != nextXtLAs[prod].end(); jj++)
						{
							string disambig = *jj;
							if (xtTbl[myState].find(disambig) != xtTbl[myState].end())
							{
								throw string("Conflict2 in look ahead for reduction of ") + prod + " using XT " + disambig + ", in state " + myStateStr;
							}
							xtSymTbl[myState].push_back(disambig);
							xtTbl[myState][disambig] = ioRdxMp[rdxCmd];
							if (inRules.find(disambig) == inRules.end())
							{
								tokens[myState].push_back(disambig);
							}
							rdxTbl[ioRdxMp[rdxCmd]] = rdxCmd;
							rdxSubSymbols[ioRdxMp[rdxCmd]] = seriesMbr;
						}
					}
					else if ((nextLAs.find(prod) != nextLAs.end()) &&
							(nextLAs[prod].size() > 0))
					{
						for (set<string>::iterator jj = nextLAs[prod].begin();
								jj != nextLAs[prod].end(); jj++)
						{
							string disambig = *jj;
							if (xtTbl[myState].find(disambig) != xtTbl[myState].end())
							{
								throw string("Conflict3 in look ahead for reduction of ") + prod + " using XT " + disambig + ", in state " + myStateStr;
							}
							xtSymTbl[myState].push_back(disambig);
							xtTbl[myState][disambig] = ioRdxMp[rdxCmd];
							if (inRules.find(disambig) == inRules.end())
							{
								tokens[myState].push_back(disambig);
							}
							rdxTbl[ioRdxMp[rdxCmd]] = rdxCmd;
							rdxSubSymbols[ioRdxMp[rdxCmd]] = seriesMbr;
						}
					}
*/
					else
					{
						throw string("No disambiguating symbol found for ") + prod + " in " + myStateStr;
					}
				}

// 4. Loop through each XT

				for (map<string, set<pair<string, pair<int, int> > > >::iterator ii = nextXts.begin();
						ii != nextXts.end(); ii++)
				{
					string xt = ii->first;
					string rdxSym = "";
					string seriesMbr = "";
					int frmLen = 0;
					bool hasShifts=false;

					for (set<pair<string, pair<int, int> > >::iterator jj = ii->second.begin();
							jj != ii->second.end(); jj++)
					{
						string prod = jj->first;
						int ln = jj->second.first;
						int pt = jj->second.second;

						if (pt >= inRules[prod][ln].size())
						{
							if ((rdxSym.length() > 0) && (rdxSym != prod))
							{
								throw string("Conflict in reduction between ") + rdxSym + " and " + prod + " on transition " + xt;
							}
							rdxSym = prod;
							if ((inModifiers.find(prod) != inModifiers.end()) &&
								(inModifiers[prod].find(ln) != inModifiers[prod].end()) &&
								(inModifiers[prod][ln].find(pt-1) != inModifiers[prod][ln].end()))
							{
								if ((frmLen > 0) ||
										((seriesMbr.length() > 0) &&
										(seriesMbr != inRules[prod][ln][pt-1])))
								{
									throw string("Redundant reduction2: prod: ") + prod;
								}
								seriesMbr = inRules[prod][ln][pt-1];
							}
							else
							{
								if (((frmLen != 0) && (frmLen != pt)) || (seriesMbr.length() > 0))
								{
									char buf[32];
									sprintf(buf, "pt:%d <> frmLen:%d", pt, frmLen);
									throw string("Redundant reduction3: prod: ") + prod + ", seriesMbr: " + seriesMbr + ", " + buf;
								}
								frmLen = pt;
							}
						}
						else
						{
							hasShifts=true;
						}
					}

// 4.1 If XT's new rule is at its end and has no conflicting shifts/rdxs then create a reduction state without L/A
					if (!hasShifts)
					{
						pair<string, pair<int, int> > rdxCmd(rdxSym, make_pair(frmLen, 0));
						if (ioRdxMp.find(rdxCmd) == ioRdxMp.end())
						{
							int rdxSt = ioStMp.size() + ioRdxMp.size();
							ioRdxMp[rdxCmd] = rdxSt;

#ifdef _DEBUG_

cout << "--------------------------------------------------------------------------------" << endl;
cout << "St:" << myState << " -[" << xt << "]->" << endl;
cout << "\tState " << rdxSt << ":" << endl;
dumpRdx(rdxCmd, 2);
cout << "--------------------------------------------------------------------------------" << endl;

#endif

						}
						if (xtTbl[myState].find(xt) != xtTbl[myState].end())
						{
							throw string("Conflict2 in look ahead for reduction of ") + rdxSym + " using XT " + xt + " in state " + myStateStr;
						}
						xtSymTbl[myState].push_back(xt);
						xtTbl[myState][xt] = ioRdxMp[rdxCmd];
						if (inRules.find(xt) == inRules.end())
						{
							tokens[myState].push_back(xt);
						}
						rdxTbl[ioRdxMp[rdxCmd]] = rdxCmd;
						rdxSubSymbols[ioRdxMp[rdxCmd]] = seriesMbr;
					}

// 4.2 Otherwise shift to recursion created state.  (Add trailing tokens)
					else
					{
						fillStates(ii->second, inRules, inModifiers);

#ifdef _DEBUG_

cout << "ST: " << myState << " -[" << xt << "]-> ..." << endl;

#endif

						xtSymTbl[myState].push_back(xt);
						xtTbl[myState][xt] = compile(ii->second, ioStMp, ioRdxMp, nextLAs, inRules, inModifiers);
						if (inRules.find(xt) == inRules.end())
						{
							tokens[myState].push_back(xt);
						}

#ifdef _DEBUG_

cout << "ST: " << myState << " -[" << xt << "]-> " << xtTbl[myState][xt] << endl;

#endif

					}
				}
			}

			return ioStMp[inPts];
		}
};

#endif
