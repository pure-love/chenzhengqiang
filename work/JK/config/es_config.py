# -*- coding: utf-8 -*-
# __author__ = chenzhengqiang
# __date__ = 2017-08-24


#query body for jiukang elasticsearch

'''QUERY_BODY={
    "query":{
        "bool":
            {
            "should":[
                {
                    "match":
                        {
                            "title":
                                {
                                    "query":"",
                                    "operator":"and",
                                    "minimum_should_match":"75%"
                                }
                        }
                },
                {
                    "match":
                        {
                            "content":
                                {
                                    "query":"",
                                    "operator":"and",
                                    "minimum_should_match":"75%"
                                }
                        }
                }
            ]
        }
    },
    "_source":
        {
            "include":["url","inputtime","title","content","is_took"]
        },
    "sort":[
        {
            "is_took":{"order":"desc"}
        },
        {
            "_score":{"order":"desc"}
        },
        {
            "_script":
                {
                    "script":"doc[\"title\"].size()",
                    "type":"number",
                    "order":"desc"
                }
        }
    ],
    "highlight":
        {
            "pre_tags":["&ltem&gt"],
            "post_tags":["&ltem&gt"],
            "fields":
                {
                    "title":{},
                    "content":{}
                }
        }
}'''


QUERY_BODY1={
    "query": {
        "bool": {
            "must": [
                { "match": { "hospital_area_code":""}},
                { "match": { "departments.ksname":""}}
                ]
        }
    }
    ,
    "_source":"hospital_id"
}


QUERY_BODY2={
    "query": {
        "bool": {
            "must": [
                { "bool": { "should":[{"match":{"hospital_area_code":"%s"}},{"match":{"pcode":"%s"}}]}},
                { "match": { "departments.ksname":"%s"}}
                ]
        }
    }
    ,
    "_source":"hospital_id"
}


