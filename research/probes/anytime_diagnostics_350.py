"""Explicit completion/certificate authority for the new350 schema only."""
import run_pair_score_341 as stats

OLD_FIELDS=tuple(stats.FIELDS)
FIELDS=(*OLD_FIELDS,'anytimeReturns','certifiedIncumbents')

def check(d):
    stats.require(set(d)==set(FIELDS) and all(type(v)is int and v>=0 for v in d.values()),'350 diagnostic schema')
    stats.require(d['completed']+d['exhausted']+d['failures']==d['supported']<=d['calls'],'350 search completion')
    stats.require(d['anytimeReturns']<=d['exhausted'],'350 false anytime authority')
    stats.require(d['anytimeReturns']<=d['improvements']<=d['completed']+d['anytimeReturns'],'350 improvement authority')
    stats.require(d['certifiedIncumbents']>=d['improvements'],'350 missing complete certificate')
    stats.require(d['failures']==0,'350 validity or certificate failure')
    for k,cap in {**stats.LIMITS,'settledLabels':32768}.items():
        stats.require(d[k]<=cap*d['supported'],'350 resource cap:'+k)

def install():
    # Replace only this process's validation schema, never frozen modules on disk.
    # Existing functions resolve these shared globals for every caller.
    stats.check_stats=check
    stats.FIELDS=FIELDS
