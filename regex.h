#ifndef _REGEX_H_
#define _REGEX_H_

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
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class NFA;

typedef ref_ptr<NFA> NfaPtr;

class NFA
{
	public:
		NFA(string inToken) : token(inToken), hasTok(true) {}
		NFA() : token(""), hasTok(false) {}

		void addTransition(char inCh, NfaPtr inNxt)
			{ transitions[inCh] = inNxt; }
		void addTransition(NfaPtr inNxt)
			{ nullTransitions.insert(inNxt); }
		void addAnyTransition(NfaPtr inNxt)
			{ anyTransitions.insert(inNxt); }
		void addNegTransition(char inCh, NfaPtr inNxt)
			{ negTransitions[inCh] = inNxt; }

		map<char, NfaPtr> transitions;
		set<NfaPtr> nullTransitions;
		set<NfaPtr> anyTransitions;
		map<char, NfaPtr> negTransitions;
		string token;
		bool hasTok;

		void dump(int inTab)
		{
			set<NfaPtr> blk;
			dump(inTab, blk);
		}
		void dump(int inTab, set<NfaPtr> inBlk)
		{
			for (int tt=0; tt<inTab; tt++) cout << "\t";
			cout << this << ((hasTok) ? string(" (") + token + ")" : "") << ":" << endl;

			set<NfaPtr> show;

			for (map<char, NfaPtr>::iterator ii = transitions.begin();
					ii != transitions.end(); ii++)
			{
				NfaPtr nn = ii->second;
				show.insert(nn);

				for (int tt=0; tt<=inTab; tt++) cout << "\t";
				cout << ii->first << ": " << (*nn) << endl;
			}
			for (set<NfaPtr>::iterator ii = nullTransitions.begin();
					ii != nullTransitions.end(); ii++)
			{
				NfaPtr nn = *ii;
				show.insert(nn);
				for (int tt=0; tt<=inTab; tt++) cout << "\t";
				cout << "<e>: " << (*nn) << endl;
			}
			for (set<NfaPtr>::iterator ii = anyTransitions.begin();
					ii != anyTransitions.end(); ii++)
			{
				NfaPtr nn = *ii;
				show.insert(nn);
				for (int tt=0; tt<=inTab; tt++) cout << "\t";
				cout << "<.>: " << (*nn) << endl;
			}
			for (map<char, NfaPtr>::iterator ii = negTransitions.begin();
					ii != negTransitions.end(); ii++)
			{
				NfaPtr nn = ii->second;
				show.insert(nn);

				for (int tt=0; tt<=inTab; tt++) cout << "\t";
				cout << "Not " << ii->first << ": " << (*nn) << endl;
			}
			for (set<NfaPtr>::iterator ii = show.begin();
					ii != show.end(); ii++)
			{
				NfaPtr nn = *ii;
				if (inBlk.find(nn) == inBlk.end())
				{
					inBlk.insert(nn);
					nn->dump(inTab, inBlk);
				}
			}
		}
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class RegexState;

typedef ref_ptr<RegexState> RgxStPtr;

class RegexState
{
	public:
		static RgxStPtr load(NfaPtr inState)
		{
			RgxStPtr ret = new RegexState();
	
			map<set<NfaPtr>, RgxStPtr> tbl;

			set<NfaPtr> stSet;
			stSet.insert(inState);
			fillAllPossStates(stSet);
			tbl[stSet] = ret;
			ret->buildState(stSet, tbl);

			return ret;
		}

		static void dump(RgxStPtr inRgxSt, int inTab)
		{
			for (int tt=0; tt<inTab; tt++) cout << "\t";
			cout << "RegexStates:" << endl;
			for (int tt=0; tt<inTab; tt++) cout << "\t";
			cout << "{" << endl;

			set<RgxStPtr> visited;
			map<RgxStPtr, int> tbl;
			dumpTbl(inRgxSt, inTab+1, tbl, visited);

			for (int tt=0; tt<inTab; tt++) cout << "\t";
			cout << "}" << endl;
		}

		static string dispCh(char inCh)
		{
			string ret="";

			char buf[16];
			if ((inCh > 32) && (inCh < 127))
			{
				sprintf(buf, "'%c'", inCh);
				ret = buf;
			}
			else
			{
				sprintf(buf, "0x%d", inCh);
				ret = buf;
			}

			return ret;
		}

		static void dumpTbl(RgxStPtr inRgxSt, int inTab, map<RgxStPtr, int> &inTbl, set<RgxStPtr> &ioVisited)
		{
			if (ioVisited.find(inRgxSt) == ioVisited.end())
			{
				ioVisited.insert(inRgxSt);

				// Load ID tables...
				if (inTbl.find(inRgxSt) == inTbl.end())
				{
					inTbl[inRgxSt] = inTbl.size();
				}

				// Print out the state...
				for (int tt=0; tt<inTab; tt++) cout << "\t";
				cout << inTbl[inRgxSt] << ((inRgxSt->hasTok) ? string(" [") + inRgxSt->token + "]" : "") << ":    (" << (*inRgxSt) << ")" << endl;

				char lst=(char) 0;
				char st=(char) 0;
				bool onCh=false;
				RgxStPtr lstRp;

				for (int chv=0; chv<128; chv++)
				{
					char ch = (char) chv;
					if (inRgxSt->transitions.find(ch) != inRgxSt->transitions.end())
					{

						RgxStPtr rgx = inRgxSt->transitions[ch];
						if (!onCh)
						{
							st = ch;
						}
						else if (lstRp != rgx)
						{
							if (inTbl.find(lstRp) == inTbl.end())
							{
								inTbl[lstRp] = inTbl.size();
							}

							for (int tt=0; tt<=inTab; tt++) cout << "\t";
							cout << dispCh(st) << ((lst > st) ? string(" - ") + dispCh(lst) : "") << " -> " << inTbl[lstRp] << endl;
							st = ch;
						}
						lstRp = rgx;
						lst = ch;
						onCh = true;
					}
					else
					{
						if (onCh)
						{
							if (inTbl.find(lstRp) == inTbl.end())
							{
								inTbl[lstRp] = inTbl.size();
							}

							for (int tt=0; tt<=inTab; tt++) cout << "\t";
							cout << dispCh(st) << ((lst > st) ? string(" - ") + dispCh(lst) : "") << " -> " << inTbl[lstRp] << endl;
						}
						onCh = false;
					}
				}
				if (onCh)
				{
					if (inTbl.find(lstRp) == inTbl.end())
					{
						inTbl[lstRp] = inTbl.size();
					}

					for (int tt=0; tt<=inTab; tt++) cout << "\t";
					cout << dispCh(st) << ((lst > st) ? string(" - ") + dispCh(lst) : "") << " -> " << inTbl[lstRp] << endl;
				}

				if (!inRgxSt->anyTransition.isNull())
				{
					if (inTbl.find(inRgxSt->anyTransition) == inTbl.end())
					{
						inTbl[inRgxSt->anyTransition] = inTbl.size();
					}

					for (int tt=0; tt<=inTab; tt++) cout << "\t";
					cout << ". -> " << inTbl[inRgxSt->anyTransition] << "   " << (*(inRgxSt->anyTransition)) << endl;
				}

				for (map<RgxStPtr, int>::iterator ii = inTbl.begin();
						ii != inTbl.end(); ii++)
				{
					if (ii->second == (inTbl[inRgxSt] + 1))
					{
						dumpTbl(ii->first, inTab, inTbl, ioVisited);
						break;
					}
				}
			}
		}

		bool hasXt(char inCh) const
		{
			bool xt = (transitions.find(inCh) != transitions.end());
			bool any = (!anyTransition.isNull());

			bool ret = xt || any;

//cout << "HAS (" << this << "): ["  << inCh << "] ? " << xt << " || " << any << ", any: " << (*anyTransition) << endl;

			return ret;
		}

		RgxStPtr next(char inCh) const
		{
			RgxStPtr ret;

			if (transitions.find(inCh) != transitions.end())
			{
				ret = ((RegexState*) this)->transitions[inCh];
			}
			else if (!anyTransition.isNull())
			{
				ret = anyTransition;
			}

			return ret;
		}

		bool hasToken() const { return hasTok; }
		string getToken() const { return token; }
	private:
		map<char, RgxStPtr> transitions;
		RgxStPtr anyTransition;
		string token;
		bool hasTok;

		RegexState()
			: hasTok(false) {}

		static void fillAllPossStates(set<NfaPtr> &ioStates)
		{
			bool addedOne=false;

			for (set<NfaPtr>::iterator ii = ioStates.begin();
					ii != ioStates.end(); ii++)
			{
				NfaPtr nn = *ii;

				for (set<NfaPtr>::iterator jj = nn->nullTransitions.begin();
						jj != nn->nullTransitions.end(); jj++)
				{
					if (ioStates.find(*jj) == ioStates.end())
					{
						ioStates.insert(*jj);
						addedOne = true;
					}
				}
			}

			if (addedOne)
			{
				fillAllPossStates(ioStates);
			}
		}

		void buildState(set<NfaPtr> inStSet, map<set<NfaPtr>, RgxStPtr> &inTbl)
		{
			map<char, set<NfaPtr> > xts;
			set<NfaPtr> anyNxts;
			set<NfaPtr> anyButNxts;

			// Gather up the 'any' and 'any but' transitions...
			for (set<NfaPtr>::iterator ii = inStSet.begin();
					ii != inStSet.end(); ii++)
			{
				NfaPtr nn = *ii;

				for (set<NfaPtr>::iterator jj = nn->anyTransitions.begin();
						jj != nn->anyTransitions.end(); jj++)
				{
					anyNxts.insert(*jj);
				}
			}

			map<NfaPtr, set<char> > negSet;
			// Gather up the 'any' and 'any but' transitions...
			for (set<NfaPtr>::iterator ii = inStSet.begin();
					ii != inStSet.end(); ii++)
			{
				NfaPtr nn = *ii;

				// Gather up all 'anything but' character transitions...
				for (map<char, NfaPtr>::iterator jj = nn->negTransitions.begin();
						jj != nn->negTransitions.end(); jj++)
				{
					char ch = jj->first;
					negSet[jj->second].insert(ch);
				}
			}
			for (map<NfaPtr, set<char> >::iterator ii = negSet.begin();
					ii != negSet.end(); ii++)
			{
				NfaPtr nn = ii->first;

				if (nn->hasTok)
				{
//					if ((hasTok) && (token != nn->token)) { throw string("Conflict: [") + nn->token + "] and [" + token + "]"; }
					token = nn->token;
					hasTok = true;
				}

				for (int chv=1; chv<128; chv++)
				{
					char ch = (char) chv;
					if (ii->second.find(ch) == ii->second.end())
					{
						xts[ch].insert(nn);

						// Also add the '.' any links...
						for (set<NfaPtr>::iterator kk = anyNxts.begin();
								kk != anyNxts.end(); kk++)
						{
							xts[ch].insert(*kk);
						}
					}
				}
			}

			// Gather up all possible character transitions...
			for (set<NfaPtr>::iterator ii = inStSet.begin();
					ii != inStSet.end(); ii++)
			{
				NfaPtr nn = *ii;

				// while I'm here, set the tokens (not really what this loop is about)...
				if (nn->hasTok)
				{
//					if ((hasTok) && (token != nn->token)) { throw string("Conflict: [") + nn->token + "] and [" + token + "]"; }
					token = nn->token;
					hasTok = true;
				}

				for (map<char, NfaPtr>::iterator jj = nn->transitions.begin();
						jj != nn->transitions.end(); jj++)
				{
					char ch = jj->first;
					xts[ch].insert(jj->second);

					// Also add the '.' any links...
					for (set<NfaPtr>::iterator kk = anyNxts.begin();
							kk != anyNxts.end(); kk++)
					{
						xts[ch].insert(*kk);
					}
				}
			}

			// For each char transition, find/make the next state.
			for (map<char, set<NfaPtr> >::iterator ii = xts.begin();
					ii != xts.end(); ii++)
			{
				char ch = ii->first;
				set<NfaPtr> nxts = ii->second;
				fillAllPossStates(nxts);
				RgxStPtr nxt;

				if (inTbl.find(nxts) != inTbl.end())
				{
					nxt = inTbl[nxts];
				}
				else
				{
					nxt = new RegexState();
					inTbl[nxts] = nxt;
					nxt->buildState(nxts, inTbl);
				}
				transitions[ch] = nxt;
			}

			// For the 'any' transition, find/make the next state.
			if (anyNxts.size() > 0)
			{
				fillAllPossStates(anyNxts);
				if (inTbl.find(anyNxts) != inTbl.end())
				{
					anyTransition = inTbl[anyNxts];
				}
				else
				{
					anyTransition = new RegexState();
					inTbl[anyNxts] = anyTransition;
					anyTransition->buildState(anyNxts, inTbl);
				}
			}

		}
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class RegexExpression
{
	public:
		virtual pair<NfaPtr, NfaPtr> generate() const =0;
};

typedef ref_ptr<RegexExpression> RePtr;

class TokRegexExpr : public RegexExpression
{
	public:
		TokRegexExpr(string inTok, RePtr inExpr) : token(inTok), expr(inExpr) {}
		pair<NfaPtr, NfaPtr> generate() const
			{
				pair<NfaPtr, NfaPtr> nn = expr->generate();
				NfaPtr en = new NFA(token);
				nn.second->addTransition(en);
				return pair<NfaPtr, NfaPtr>(nn.first, en); }
	private:
		string token;
		RePtr expr;
};

class ChrRegexExpr : public RegexExpression
{
	public:
		ChrRegexExpr(const char inCh) : ch(inCh) {}
		pair<NfaPtr, NfaPtr> generate() const
			{
				NfaPtr st = new NFA(), en = new NFA();
				st->addTransition(ch, en);
				return pair<NfaPtr, NfaPtr>(st, en); }
	private:
		const char ch;
};

class AnyRegexExpr : public RegexExpression
{
	public:
		pair<NfaPtr, NfaPtr> generate() const
			{
				NfaPtr st = new NFA(), en = new NFA();
				st->addAnyTransition(en);
				return pair<NfaPtr, NfaPtr>(st, en);
			}
};

class ChrRngRegexExpr : public RegexExpression
{
	public:
		ChrRngRegexExpr(char inCMin, char inCMax) : isPos(true)
		{
			for (char cc = inCMin; cc <= inCMax; cc++)
			{
				rng.insert(cc);
			}
		}
		ChrRngRegexExpr(char inCMin, char inCMax, bool inIsPos) : isPos(inIsPos)
		{
			for (char cc = inCMin; cc <= inCMax; cc++)
			{
				rng.insert(cc);
			}
		}

		ChrRngRegexExpr* add(char inCMin, char inCMax)
		{
			for (char cc = inCMin; cc <= inCMax; cc++)
			{
				rng.insert(cc);
			}
			return this;
		}

		pair<NfaPtr, NfaPtr> generate() const
		{
			NfaPtr st = new NFA(), en = new NFA();

			for (set<char>::iterator ii = rng.begin();
					ii != rng.end(); ii++)
			{
				if (isPos)
				{
					st->addTransition(*ii, en);
				}
				else
				{
					st->addNegTransition(*ii, en);
				}
			}

			return pair<NfaPtr, NfaPtr>(st, en);
		}
	private:
		set<char> rng;
		bool isPos;
};

class ListRegexExpr : public RegexExpression
{
	public:
		ListRegexExpr* add(RePtr inElem)
			{
				elems.push_back(inElem);
				return this;
			}
			pair<NfaPtr, NfaPtr> generate() const
			{
				NfaPtr st, en;
				for (vector<RePtr>::const_iterator ii = elems.begin(); ii != elems.end(); ii++)
				{
					pair<NfaPtr, NfaPtr> ee = (*ii)->generate();

					if (st.isNull())
					{
						st = ee.first;
						en = ee.second;
					}
					else
					{
						en->addTransition(ee.first);
						en = ee.second;
					}
				}
				if (st.isNull())
				{
					NfaPtr aa = new NFA(), bb = new NFA();
					aa->addTransition(bb);
					return pair<NfaPtr, NfaPtr>(aa, bb);
				}
				return pair<NfaPtr, NfaPtr>(st, en);
			}
	private:
		vector<RePtr> elems;
};

class SetRegexExpr : public RegexExpression
{
	public:
		SetRegexExpr* add(RePtr inElem)
			{
				elems.insert(inElem);
				return this;
			}
			pair<NfaPtr, NfaPtr> generate() const
			{
				NfaPtr st = new NFA(), en = new NFA();

				for (set<RePtr>::const_iterator ii = elems.begin(); ii != elems.end(); ii++)
				{
					pair<NfaPtr, NfaPtr> ee = (*ii)->generate();

					st->addTransition(ee.first);
					ee.second->addTransition(en);
				}
				if (elems.size() == 0)
				{
					st->addTransition(en);
				}
				return pair<NfaPtr, NfaPtr>(st, en);
			}
		const bool operator<(const RegexExpression& inArg) const {return this < &inArg; }
	private:
		set<RePtr> elems;
};

class SeriesRegexExpr : public RegexExpression
{
	public:
		SeriesRegexExpr(int inMin, RePtr inElem, int inMax)
			: elem(inElem), min(inMin), max(inMax) {}
		SeriesRegexExpr(int inMin, RePtr inElem)
			: elem(inElem), min(inMin), max(-1) {}
		SeriesRegexExpr(RePtr inElem, int inMax)
			: elem(inElem), min(0), max(inMax) {}
		SeriesRegexExpr(RePtr inElem)
			: elem(inElem), min(0), max(-1) {}
		pair<NfaPtr, NfaPtr> generate() const
		{
			NfaPtr st = new NFA(), en = new NFA();

			NfaPtr pt = st;
			NfaPtr lst = st;

			if ((min < 1) && (max < 0))
			{
				st->addTransition(en);
				pair<NfaPtr, NfaPtr> ee = elem->generate();
				pt->addTransition(ee.first);
				lst = ee.first;
				pt = ee.second;
			}

			for (int ii=0; ii<min; ii++)
			{
				pair<NfaPtr, NfaPtr> ee = elem->generate();
				pt->addTransition(ee.first);
				lst = ee.first;
				pt = ee.second;
			}

			if (max < 0)
			{
				pt->addTransition(lst);
			}
			else
			{
				for (int ii=0; ii<(max - min); ii++)
				{
					pair<NfaPtr, NfaPtr> ee = elem->generate();
					pt->addTransition(ee.first);
					pt->addTransition(en);
					pt = ee.second;
				}
			}

			pt->addTransition(en);

			return pair<NfaPtr, NfaPtr>(st, en);
		}
	private:
		RePtr elem;
		int min;
		int max;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class Pipeline
{
	public:
		virtual int getRootStateId() const =0;
		virtual bool hasNextSymbol() const =0;
		virtual bool hasNextSymbol(int inSt) const =0;
		virtual symPtr getNextSymbol() =0;
};

typedef ref_ptr<Pipeline> pipelinePtr;

class RegexPipeline : public Pipeline
{
	public:
		RegexPipeline(istream &inStream, RgxStPtr inRgxSt)
			: strm(inStream) { states[0] = inRgxSt; }
		RegexPipeline(istream &inStream)
			: strm(inStream) {}

		void addState(int inSt, RgxStPtr inRgxSt)
		{
			states[inSt] = inRgxSt;
		}

		void addIgnore(string inTok)
		{
			addIgnore(0, inTok);
		}

		void addIgnore(int inSt, string inTok)
		{
			ignore[inSt].insert(inTok);
		}

		int getRootStateId() const { return 0; }

		bool hasNextSymbol() const
		{
			return hasNextSymbol(0);
		}

		bool hasNextSymbol(int inSt) const
		{
			if (nxt.isNull())
			{
				bool gotOne = false;
				while (!gotOne)
				{
					RgxStPtr cur = ((RegexPipeline*) this)->states[inSt];
					string txt = "";
					int pt=0, lastTokPt=0;
					char buf[512];
					int bPt=0, bLen=0;

					symPtr none;
					((RegexPipeline*) this)->nxt = none;

					char ch;

					while ((strm.get(ch)) && (cur->hasXt(ch)))
					{
						cur = cur->next(ch);
						txt += ch;
						if (cur->hasToken())
						{
							((RegexPipeline*) this)->nxt = new Symbol(cur->getToken(), txt);
							lastTokPt = pt;
						}
						pt++;
					}
					for (int ii=0; ii<txt.length() - lastTokPt; ii++)
					{
						strm.unget();
					}

					if ((nxt.isNull()) ||
						(((RegexPipeline*) this)->ignore[inSt].find(nxt->getName()) == ((RegexPipeline*) this)->ignore[inSt].end()))
					{
						gotOne = true;
					}
				}
			}

			return (!nxt.isNull());
		}
		symPtr getNextSymbol()
		{
			symPtr ret = nxt;
			symPtr none;
			nxt = none;
			return ret;
		}
	private:
		istream &strm;
		map<int, RgxStPtr> states;
		symPtr nxt;
		map<int, set<string> > ignore;
};

#endif
