#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iostream>

extern "C" {
#include <vm/interp.h>
}

#include "script.hpp"
#include "dict.hpp"

using namespace std;

ScriptEngine se;

static void __dict_enumerate(object_t object, void *list, void(*list_add)(void *, object_t)) { } 
static void __dict_free(object_t object) {
    delete (Dict *)object->external.priv;
}

struct see_external_type_s external_type_dict =
{
    "DICT",
    __dict_enumerate,
    __dict_free,
};

object_t playerDict;
object_t actionDict;

#define DECLARE_EXFUNC(name) \
    static object_t EXFUNC_##name(void *priv, int argc, object_t *argv)
#define EXFUNC     argv[0]
#define EXARG(idx) argv[(idx) + 1]
#define ENGINE     ((ScriptEngine *)priv)

DECLARE_EXFUNC(GetPlayerDict)
{
    return playerDict;
}

DECLARE_EXFUNC(GetActionDict)
{
    return actionDict;
}

DECLARE_EXFUNC(DictLoad)
{
    if (argc < 2) return OBJECT_NULL;
    
    object_t filename = EXARG(0);
    if (OBJECT_TYPE(filename) != OBJECT_TYPE_STRING)
        return OBJECT_NULL;
    if (strpbrk(xstring_cstr(filename->string), "\\/"))
        return OBJECT_NULL;

    FILE *f = fopen(xstring_cstr(filename->string), "r");
    if (f == NULL)
        return OBJECT_NULL;

    Dict *d = new Dict();
    if (d->DeserializeFromFile(f))
    {
        fclose(f);
        delete d;
        return OBJECT_NULL;
    }
    
    fclose(f);

    d->Remove("__name");

    object_t result = ENGINE->ObjectNew();
    
    result->external.priv = d;
    result->external.type = &external_type_dict;
    OBJECT_TYPE_INIT(result, OBJECT_TYPE_EXTERNAL);

    return result;
}

DECLARE_EXFUNC(DictSave)
{
    if (argc < 2) return OBJECT_NULL;

    object_t dict = EXARG(0);
    if (OBJECT_TYPE(dict) != OBJECT_TYPE_EXTERNAL ||
        dict->external.type != &external_type_dict)
        return OBJECT_NULL;

    Dict *d =  ((Dict *)dict->external.priv);
    string fn;
    
    if (d->Get("__name", fn) == 0)
    {
        FILE *f = fopen(fn.c_str(), "w");
        if (f)
        {
            d->SerializeToFile(f);
            fclose(f);
        }
    }

    return OBJECT_NULL;
}

DECLARE_EXFUNC(DictGet)
{
    if (argc < 3) return OBJECT_NULL;

    object_t dict = EXARG(0);
    object_t key  = EXARG(1);
    if (OBJECT_TYPE(dict) != OBJECT_TYPE_EXTERNAL ||
        OBJECT_TYPE(key)  != OBJECT_TYPE_STRING ||
        dict->external.type != &external_type_dict)
        return OBJECT_NULL;

    object_t result;
    string value;

    if (((Dict *)dict->external.priv)->Get(xstring_cstr(key->string), value))
    {
        result = OBJECT_NULL;
    }
    else
    {
        result = ENGINE->ObjectNew();
        result->string = xstring_from_cstr(value.c_str(), value.length());
        OBJECT_TYPE_INIT(result, OBJECT_TYPE_STRING);
    }

    return result;
}

DECLARE_EXFUNC(DictPut)
{
    if (argc < 4) return OBJECT_NULL;
        
    object_t dict  = EXARG(0);
    object_t key   = EXARG(1);
    object_t value = EXARG(2);
    if (OBJECT_TYPE(dict)   != OBJECT_TYPE_EXTERNAL ||
        OBJECT_TYPE(key)    != OBJECT_TYPE_STRING ||
        OBJECT_TYPE(value)  != OBJECT_TYPE_STRING ||
        dict->external.type != &external_type_dict)
        return OBJECT_NULL;

    if (strcmp(xstring_cstr(key->string), "__name") == 0) return OBJECT_NULL;

    ((Dict *)dict->external.priv)->Put(xstring_cstr(key->string), xstring_cstr(value->string));
    return OBJECT_NULL;
}

DECLARE_EXFUNC(DictDel)
{
    if (argc < 3) return OBJECT_NULL;

    object_t dict = EXARG(0);
    object_t key  = EXARG(1);
    if (OBJECT_TYPE(dict) != OBJECT_TYPE_EXTERNAL ||
        OBJECT_TYPE(key)  != OBJECT_TYPE_STRING ||
        dict->external.type != &external_type_dict)
        return OBJECT_NULL;

    if (strcmp(xstring_cstr(key->string), "__name") == 0) return OBJECT_NULL;

    ((Dict *)dict->external.priv)->Remove(xstring_cstr(key->string));
    return OBJECT_NULL;
}

DECLARE_EXFUNC(DictNextKey)
{
    if (argc < 3) return OBJECT_NULL;

    object_t dict   = EXARG(0);
    object_t key    = EXARG(1);
    object_t prefix = OBJECT_NULL;
    
    if (OBJECT_TYPE(dict) != OBJECT_TYPE_EXTERNAL ||
        OBJECT_TYPE(key)  != OBJECT_TYPE_STRING ||
        dict->external.type != &external_type_dict)
        return OBJECT_NULL;

    if (argc > 3 && OBJECT_TYPE(EXARG(2)) == OBJECT_TYPE_STRING)
    {
        prefix = EXARG(2);
    }

    object_t result;
    string nkey;
    
    if (((Dict *)dict->external.priv)->GetNextKey(xstring_cstr(key->string), prefix == OBJECT_NULL ? "" : xstring_cstr(prefix->string), nkey))
    {
        result = OBJECT_NULL;
    }
    else
    {
        result = ENGINE->ObjectNew();
        result->string = xstring_from_cstr(nkey.c_str(), nkey.length());
        OBJECT_TYPE_INIT(result, OBJECT_TYPE_STRING);
    }

    return result;
}


static void
WriteObject(object_t object, ostream &o)
{
    switch (OBJECT_TYPE(object))
    {
    case OBJECT_TYPE_STRING:
        o << xstring_cstr(object->string);
        break;

    case ENCODE_SUFFIX_INT:
        o << INT_UNBOX(object);
        break;
    }
}

DECLARE_EXFUNC(StringEqP)
{
    if (argc < 3)
        return OBJECT_NULL;
        
    object_t a = EXARG(0);
    object_t b = EXARG(1);
    
    if (OBJECT_TYPE(a) != OBJECT_TYPE_STRING ||
        OBJECT_TYPE(b) != OBJECT_TYPE_STRING ||
        !xstring_equal(a->string, b->string))
        return OBJECT_FALSE;

    return OBJECT_TRUE;
}

DECLARE_EXFUNC(ToString)
{
    ostringstream o;
    int i;
    for (i = 1; i < argc; ++ i)
    {
        WriteObject(argv[i], o);
    }

    string value = o.str();
    object_t result = ENGINE->ObjectNew();
    result->string = xstring_from_cstr(value.c_str(), value.length());
    OBJECT_TYPE_INIT(result, OBJECT_TYPE_STRING);

    return result;
}

DECLARE_EXFUNC(Debug)
{
    int i;
    for (i = 1; i < argc; ++ i)
    {
        object_dump(argv[i], stderr);
    }
    fprintf(stderr, "\n");

    return OBJECT_NULL;
}


DECLARE_EXFUNC(Output)
{
    int i;
    for (i = 1; i < argc; ++ i)
    {
        WriteObject(argv[i], cout);
    }
    return OBJECT_NULL;
}

int
main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s player-data-file-name", argv[0]);
        return -1;
    }

    FILE *pdataf = NULL;

    if (strpbrk(argv[1], "\\/\n\r") == NULL)
    {
        pdataf = fopen(argv[1], "r");
    }

    if (pdataf == NULL)
    {
        fprintf(stderr, "Cannot open file %s\n", argv[1]);
        return -1;
    }

    Dict *pdata = new Dict();    
    if (pdata->DeserializeFromFile(pdataf))
    {
        fprintf(stderr, "Cannot parse player data\n");
        fclose(pdataf);
        delete pdata;

        return -1;
    }

    fclose(pdataf);

    Dict *action = new Dict();
    action->DeserializeFromFile(stdin);

    string act;
    if (pdata->Get("action_script", act))
    {
        fprintf(stderr, "No action script in player data\n");
        delete pdata;
        return -1;
    }

    pdata->Put("__name", argv[1]);
    action->Remove("__name");

    object_t script = se.LoadScript(act.c_str());

    playerDict = se.ObjectNew();
    playerDict->external.priv = pdata;
    playerDict->external.type = &external_type_dict;
    OBJECT_TYPE_INIT(playerDict, OBJECT_TYPE_EXTERNAL);

    actionDict = se.ObjectNew();
    actionDict->external.priv = action;
    actionDict->external.type = &external_type_dict;
    OBJECT_TYPE_INIT(actionDict, OBJECT_TYPE_EXTERNAL);

    se.ExternalFuncRegister("GetPlayerDict", EXFUNC_GetPlayerDict, &se);
    se.ExternalFuncRegister("GetActionDict", EXFUNC_GetActionDict, &se);
    se.ExternalFuncRegister("DictLoad", EXFUNC_DictLoad, &se);
    se.ExternalFuncRegister("DictSave", EXFUNC_DictSave, &se);
    se.ExternalFuncRegister("DictGet",  EXFUNC_DictGet, &se);
    se.ExternalFuncRegister("DictPut",  EXFUNC_DictPut, &se);
    se.ExternalFuncRegister("DictDel",  EXFUNC_DictDel, &se);
    se.ExternalFuncRegister("DictNextKey", EXFUNC_DictNextKey, &se);

    se.ExternalFuncRegister("StringEq?", EXFUNC_StringEqP, &se);
    se.ExternalFuncRegister("ToString", EXFUNC_ToString, &se);
    se.ExternalFuncRegister("Debug",    EXFUNC_Debug, &se);
    se.ExternalFuncRegister("Output",   EXFUNC_Output, &se);
    
    object_t exret;
    std::vector<object_t> excall;
    excall.clear();
    se.Apply(script, &excall);
    while (1)
    {
        int r = se.Execute(exret, &excall);
        if (r == APPLY_EXTERNAL_CALL)
        {
            exret = OBJECT_NULL;
            for (size_t i = 0; i < excall.size(); ++ i)
            {
                se.ObjectUnprotect(excall[i]);
            }
        }
        else break;
    }

    return 0;
}
