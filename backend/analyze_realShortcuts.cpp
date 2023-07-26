#include "analyze_realShortcuts.hpp"

Analyze_realShortcuts::Analyze_realShortcuts()
{

}

/*
 * Analyss or real shortcuts
 * We take al 'big' peers (>1BTC capacity?)
 * We create an abbacus of the cost to reach each pair in each direction
 * We keep only the pairs where the smalest of the two costs is >0 (to be sure we are able to have a lower cost than everyone
 * Then We take the one with the biggest cost on the other side?
 * Biggest cost should imply interest in routing. But it might also imply that there will be no route at all the other way
 *
 */
