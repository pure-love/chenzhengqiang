# -*- coding: utf-8 -*-
# __author__ = chenzhengqiang
# __date__ = 2017-09-13




HOSPITALS_QUERY={
        "query":
            {
                "bool": {
                    "must": {
                            "multi_match":
                                {
                                    "query": "心律失常",
                                    "type":"best_fields",
                                    "fields": ["doctors.advantage","departments.ksname","departments.catname"],
                                    "minimum_should_match": "80%"
                                }
                            },
                    "filter":{"bool":{"should":
                            [
                                {
                                    "term": {"hospital_area_code": "1101"}
                                },
                                {
                                    "term": {"pcode": 1123}
                                }
                            ]
                        }}
                }
            },
            "_source":"hospital_id"
}


