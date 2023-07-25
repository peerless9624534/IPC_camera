#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include <imp_log.h>

#include <iniparser.h>
#include <lux_iniparse.h>

#define     INI_FILES_MAX   16
#define     INI_NAME_LEN    64
#define     TAG             "lux_iniparse"

typedef struct 
{
    BOOL_X  bUsed;
    CHAR_X  name[INI_NAME_LEN];
    dictionary  *ini;

}LUX_INIPARSE_ATTR_ST;

LUX_INIPARSE_ATTR_ST stIniAttr[INI_FILES_MAX];

/**
 * @description: 获取配置文件属性
 * @param [in]	pFileName	文件名称
 * @return 成功返回字典属性指针地址， 错误返回NULL
 */
static LUX_INIPARSE_ATTR_ST *LUX_INIParse_GetDicAttr(const CHAR_X *pFileName)
{
    if(NULL == pFileName)
    {
        IMP_LOG_ERR(TAG, "LUX_INIParse_GetDicAttr failed!\n");
		return NULL;
    }

    INT_X i = 0;

    for(i = 0; i < INI_FILES_MAX; i++)
    {
        if(stIniAttr[i].bUsed)
        {
           if(!strcmp(stIniAttr[i].name, pFileName))
           {
               return &stIniAttr[i];
           }
        }
    }

    /*未找到的异常情况*/
    if(INI_FILES_MAX == i)
    {
        IMP_LOG_ERR(TAG, "Not found %s, Please load it before used!\n", pFileName);
    }

    return NULL;
}


/**
 * @description: 加载解析ini文件
 * @param [in]	pFileName	文件名称
 * @return 0 成功， 非零失败，返回错误码
 */
INT_X LUX_INIParse_Load(const CHAR_X *pFileName)
{
    if(NULL == pFileName)
    {
        IMP_LOG_ERR(TAG, "LUX_INIParse_Load failed.\n");
		return LUX_PARM_NULL_PTR;
    }

    INT_X i = 0;
    CHAR_X tmpPath[64] = {0};

    sprintf(tmpPath, "%s/%s", INI_FILE_PATH, pFileName);
 
    for(i = 0; i < INI_FILES_MAX; i++)
    {
        if(!stIniAttr[i].bUsed)
        {
            stIniAttr[i].ini = iniparser_load(tmpPath);
            if(NULL == stIniAttr[i].ini)
            {
                IMP_LOG_ERR(TAG, "iniparser_load failed!\n");
                return LUX_PARM_NULL_PTR;
            }
            strcpy(stIniAttr[i].name, pFileName);
            stIniAttr[i].bUsed = LUX_TRUE;
            break;
        }
    }

    if(INI_FILES_MAX == i)
    {
        IMP_LOG_ERR(TAG, "LUX_INIParse_Load failed, load files full!\n");
        return LUX_INI_LOAD_FILE_FULL;
    }

    return LUX_OK;
}


/**
 * @description: 获取键的布尔值
 * @param [in]	pFileName	文件名称
 * @param [in]	pTitle   	标题
 * @param [in]	pKey	    键
 * @param [out] value       获取到的值
 * @return 成功返回0， 错误返回错误码
 */
INT_X LUX_INIParse_GetBool(const PCHAR_X pFilename, const PCHAR_X pTitle, const PCHAR_X pKey, BOOL_X *value)
{
    if(NULL == pFilename || NULL == pTitle || NULL == pKey || NULL == value)
    {
        IMP_LOG_ERR(TAG, "LUX_INIParse_GetBool failed.\n");
		return LUX_PARM_NULL_PTR;
    }

    LUX_INIPARSE_ATTR_ST *pAttr = NULL;
    CHAR_X tmpStr[128] = {0};
    INT_X ret = LUX_ERR;

    pAttr= LUX_INIParse_GetDicAttr(pFilename);
    if(NULL == pAttr)
    {
        IMP_LOG_ERR(TAG, "LUX_INIParse_GetDicAttr failed, flie name : [%s]\n", pFilename);
        return LUX_INI_GET_DIC_ATTR_FAILED;
    }

    sprintf(tmpStr, "%s:%s", pTitle, pKey);

    /*获取错误返回-1*/
    ret = iniparser_getboolean(pAttr->ini, tmpStr, LUX_ERR);
    if(LUX_ERR == ret)
    {
        IMP_LOG_ERR(TAG, "iniparser_getboolean failed!,file[%s], title[%s], key[%s]\n", pFilename, pTitle, pKey);
        return LUX_INI_GET_PARAM_FAILED;
    }

    *value = (BOOL_X)ret;

    return LUX_OK;  
}


/**
 * @description: 获取键的整形值
 * @param [in]	pFileName	文件名称
 * @param [in]	pTitle   	标题
 * @param [in]	pKey	    键
 * @param [out] value       获取到的值
 * @return 成功返回0， 错误返回错误码
 * @attention -2^31这个值被用于找不到返回错误码
 */
INT_X LUX_INIParse_GetInt(const PCHAR_X pFilename, const PCHAR_X pTitle, const PCHAR_X pKey, PINT_X value)
{
    if(NULL == pFilename || NULL == pTitle || NULL == pKey || NULL == value)
    {
        IMP_LOG_ERR(TAG, "LUX_INIParse_GetInt failed.\n");
		return LUX_PARM_NULL_PTR;
    }

    LUX_INIPARSE_ATTR_ST *pAttr = NULL;
    CHAR_X tmpStr[128] = {0};
    INT_X err = (INT_X)1 << 31;
    INT_X ret = err;

    pAttr= LUX_INIParse_GetDicAttr(pFilename);
    if(NULL == pAttr)
    {
        IMP_LOG_ERR(TAG, "LUX_INIParse_GetDicAttr failed, flie name : [%s]\n", pFilename);
        return LUX_INI_GET_DIC_ATTR_FAILED;
    }

    sprintf(tmpStr, "%s:%s", pTitle, pKey);

    /*获取错误返回-1*/
    ret = iniparser_getint(pAttr->ini, tmpStr, err);
    if(err == ret)
    {
        IMP_LOG_ERR(TAG, "iniparser_getint failed!,file[%s], title[%s], key[%s]\n", pFilename, pTitle, pKey);
        return LUX_INI_GET_PARAM_FAILED;
    }

    *value = ret;

    return LUX_OK;  
}

/**
 * @description: 获取键的浮点值
 * @param [in]	pFileName	文件名称
 * @param [in]	pTitle   	标题
 * @param [in]	pKey	    键
 * @param [out] value       获取到的值
 * @return 成功返回0， 错误返回错误码
 * @attention DBL_MAX这个值被用于找不到返回错误码
 */
INT_X LUX_INIParse_GetDouble(const PCHAR_X pFilename, const PCHAR_X pTitle, const PCHAR_X pKey, PDOUBLE_X value)
{
    if(NULL == pFilename || NULL == pTitle || NULL == pKey || NULL == value)
    {
        IMP_LOG_ERR(TAG, "LUX_INIParse_GetDouble failed.\n");
		return LUX_PARM_NULL_PTR;
    }

    LUX_INIPARSE_ATTR_ST *pAttr = NULL;
    CHAR_X tmpStr[128] = {0};
    DOUBLE_X err = DBL_MAX;
    DOUBLE_X ret = err;

    pAttr= LUX_INIParse_GetDicAttr(pFilename);
    if(NULL == pAttr)
    {
        IMP_LOG_ERR(TAG, "LUX_INIParse_GetDicAttr failed, flie name : [%s]\n", pFilename);
        return LUX_INI_GET_DIC_ATTR_FAILED;
    }

    sprintf(tmpStr, "%s:%s", pTitle, pKey);

    /*获取错误返回-1*/
    ret = iniparser_getdouble(pAttr->ini, tmpStr, err);
    if(err == ret)
    {
        IMP_LOG_ERR(TAG, "iniparser_getdouble failed!,file[%s], title[%s], key[%s]\n", pFilename, pTitle, pKey);
        return LUX_INI_GET_PARAM_FAILED;
    }

    *value = ret;

    return LUX_OK;  
}

/**
 * @description: 获取键的字符类型值
 * @param [in]	pFileName	文件名称
 * @param [in]	pTitle   	标题
 * @param [in]	pKey	    键
 * @param [out] pOutStr     获取到字符串
 * @return 成功返回0，错误返回错误码
 */
INT_X LUX_INIParse_GetString(const PCHAR_X pFilename, const PCHAR_X pTitle, const PCHAR_X pKey, PCHAR_X pOutStr)
{
    if(NULL == pFilename || NULL == pTitle || NULL == pKey || NULL == pOutStr)
    {
        IMP_LOG_ERR(TAG, "LUX_INIParse_GetString failed.\n");
		return LUX_PARM_NULL_PTR;
    }

    LUX_INIPARSE_ATTR_ST *pAttr = NULL;
    CHAR_X tmpStr[128] = {0};
    PCHAR_X pTemGetStr = NULL;

    pAttr= LUX_INIParse_GetDicAttr(pFilename);
    if(NULL == pAttr)
    {
        IMP_LOG_ERR(TAG, "LUX_INIParse_GetDicAttr failed, flie name : [%s]\n", pFilename);
        return LUX_INI_GET_DIC_ATTR_FAILED;
    }

    sprintf(tmpStr, "%s:%s", pTitle, pKey);
 
    /*获取错误返回0*/
    pTemGetStr = iniparser_getstring(pAttr->ini, tmpStr, NULL);
    if(NULL == pTemGetStr)
    {
        IMP_LOG_ERR(TAG, "iniparser_getstring failed!,file[%s], title[%s], key[%s]\n", pFilename, pTitle, pKey);
        return LUX_INI_GET_PARAM_FAILED;
    }

    strcpy(pOutStr, pTemGetStr);

    return LUX_OK;
}


/**
 * @description: 设置键的值
 * @param [in]	 pFileName	文件名称
 * @param [in]	 pTitle   	标题
 * @param [in]	 pKey	    键
 * @param [in]  pinStr      需要设置的值
 * @return 成功返回0，错误返回错误码
 */
INT_X LUX_INIParse_SetKey(const PCHAR_X pFilename, const PCHAR_X pTitle, const PCHAR_X pKey, PCHAR_X pValue)
{

    if(NULL == pFilename || NULL == pTitle || NULL == pKey || NULL == pValue )
    {
        IMP_LOG_ERR(TAG, "failed, empty point.\n");
		return LUX_PARM_NULL_PTR;
    }

    LUX_INIPARSE_ATTR_ST *pAttr = NULL;
    CHAR_X tmpStr[128] = {0};
    INT_X ret = LUX_ERR;
    CHAR_X tmpPath[64] = {0};
    FILE* fp;

    sprintf(tmpPath, "%s/%s", INI_FILE_PATH, pFilename);

    pAttr = LUX_INIParse_GetDicAttr(pFilename);
    if (NULL == pAttr)
    {
        IMP_LOG_ERR(TAG, "LUX_INIParse_GetDicAttr failed, flie name : [%s]\n", pFilename);
        return LUX_INI_GET_DIC_ATTR_FAILED;
    }

    sprintf(tmpStr, "%s:%s", pTitle, pKey);

    ret = iniparser_set(pAttr->ini, tmpStr, pValue);
    if(LUX_OK != ret)
    {
        IMP_LOG_ERR(TAG, "iniparser_set failed!, title[%s], key[%s]\n", pTitle, pKey);
        return LUX_INI_SET_PARAM_FAILED;
    }

    fp = fopen(tmpPath, "w");
    if(NULL == fp)
    {
        IMP_LOG_ERR(TAG, "fopen failed!\n");
        return LUX_INI_OPEN_FILE_FAILED;
    }

    /*将修改后的dictionary保存到指定的文件中去*/
    iniparser_dump_ini(pAttr->ini, fp);

    fclose(fp);

    return LUX_OK;
}

/**
 * @description: 释放dictionary申请的资源
 * @param [in]	 pFileName	文件名称
 * @return 成功返回0，错误返回错误码
 */
INT_X LUX_INIParse_FreeDict(const PCHAR_X pFilename)
{
    if(NULL == pFilename)
    {
        IMP_LOG_ERR(TAG, "LUX_INIParse_FreeDict failed.\n");
		return LUX_PARM_NULL_PTR;
    }

    LUX_INIPARSE_ATTR_ST *pAttr = NULL;

    pAttr = LUX_INIParse_GetDicAttr(pFilename);
    if(NULL == pAttr)
    {
        IMP_LOG_ERR(TAG, "LUX_INIParse_GetDicAttr failed!\n");
        return LUX_INI_GET_DIC_ATTR_FAILED;
    }

    iniparser_freedict(pAttr->ini);
    memset(pAttr, 0, sizeof(LUX_INIPARSE_ATTR_ST));

    return LUX_OK;

}

 


