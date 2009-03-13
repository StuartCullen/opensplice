
#include "gapi.h"
#include "gapi_dataReaderView.h"
#include "gapi_dataReader.h"
#include "gapi_qos.h"
#include "gapi_typeSupport.h"
#include "gapi_subscriber.h"
#include "gapi_topicDescription.h"
#include "gapi_topic.h"
#include "gapi_condition.h"
#include "gapi_domainParticipant.h"
#include "gapi_domainParticipantFactory.h"
#include "gapi_structured.h"
#include "gapi_objManag.h"
#include "gapi_kernel.h"
#include "gapi_error.h"

#include "os_heap.h"
#include "u_user.h"
#include "u_instanceHandle.h"
#include "u_dataView.h"
#include "u_dataViewQos.h"
#include "v_kernel.h"
#include "v_state.h"
#include "kernelModule.h"


static gapi_boolean
copyReaderViewQosIn (
    const gapi_dataReaderViewQos *srcQos,
    v_dataViewQos dstQos)
{
    gapi_boolean copied = TRUE;

    dstQos->userKey.enable = srcQos->view_keys.use_key_list;
    if ( srcQos->view_keys.use_key_list ) {
        dstQos->userKey.expression = gapi_stringSeq_to_String(&srcQos->view_keys.key_list, ",");
        if ( (srcQos->view_keys.key_list._length > 0UL) && !dstQos->userKey.expression ) {
            assert(FALSE);
            copied = FALSE;
        }
    } else {
        dstQos->userKey.expression = NULL;
    }

    return copied;
}

static gapi_boolean
copyReaderViewQosOut (
    const v_dataViewQos srcQos,
    gapi_dataReaderViewQos *dstQos)
{
    gapi_boolean copied = TRUE;

    dstQos->view_keys.use_key_list = srcQos->userKey.enable;
    if ( srcQos->userKey.enable ) {
        copied = gapi_string_to_StringSeq(srcQos->userKey.expression, ",", &dstQos->view_keys.key_list);
    } else {
        gapi_stringSeq_set_length(&dstQos->view_keys.key_list, 0L);
    }
    return copied;
}

static gapi_boolean
initViewQuery (
    _DataReaderView dataReaderView)
{
    gapi_boolean result = FALSE;
    gapi_expression expr;
    u_dataView uReaderView;
    
    dataReaderView->reader_mask.sampleStateMask   = (c_long)0;
    dataReaderView->reader_mask.viewStateMask     = (c_long)0;
    dataReaderView->reader_mask.instanceStateMask = (c_long)0;
    
    uReaderView = u_dataView(U_DATAREADERVIEW_GET(dataReaderView));
    expr = gapi_createReadExpression(u_entity(uReaderView),
                                     &dataReaderView->reader_mask);
    if ( expr != NULL ) {
        dataReaderView->uQuery = gapi_expressionCreateQuery(expr,
                                                            u_reader(uReaderView),
                                                            NULL,
                                                            NULL);
        gapi_expressionFree(expr);
        if ( dataReaderView->uQuery ) {
            result = TRUE;
        }
    }
    return result;
}
    

_DataReaderView
_DataReaderViewNew (
    const gapi_dataReaderViewQos * qos,
    const _DataReader datareader)
{
    _DataReaderView _this;
    v_dataViewQos ViewQos;
    u_dataView uReaderView;
    _DomainParticipant participant;
    _TypeSupport typeSupport;

    _this = _DataReaderViewAlloc();
        
    if ( _this != NULL ) {
        participant = _DomainEntityParticipant(_DomainEntity(datareader));        
        _DomainEntityInit(_DomainEntity(_this),
                          participant,
                          _Entity(datareader),
                          TRUE);
         
        typeSupport = _TopicDescriptionGetTypeSupport(datareader->topicDescription);

        assert(typeSupport);
        _this->datareader = datareader;
        ViewQos = u_dataViewQosNew(NULL);
        if ( ViewQos != NULL ) {
            if ( !copyReaderViewQosIn(qos, ViewQos) ) { 
                u_dataViewQosFree(ViewQos);
                _DomainEntityDispose(_DomainEntity(_this));
                _this = NULL;
            }
        } else {
            _DomainEntityDispose(_DomainEntity(_this));
            _this = NULL;
        }
    }

    if ( _this != NULL ) {
        uReaderView = u_dataViewNew(u_dataReader(_EntityUEntity (datareader)),
                                    "dataReaderView",
                                    ViewQos);
        if ( uReaderView ) {
            U_DATAREADERVIEW_SET(_this, uReaderView);
        } else {
            _DomainEntityDispose(_DomainEntity(_this));
            _this = NULL;
        }
        u_dataViewQosFree(ViewQos);
    }

    if ( _this != NULL ) {
        _this->conditionSet = gapi_setNew (gapi_objectRefCompare);
        if ( _this->conditionSet == NULL ) {
            _DomainEntityDispose(_DomainEntity(_this));
            _this = NULL;
        }
    }
             
    if ( _this != NULL ) {
        if ( !initViewQuery(_this) ) {
            gapi_setFree(_this->conditionSet);
            u_dataViewFree(uReaderView);
            _DomainEntityDispose(_DomainEntity(_this));
            _this = NULL;
        }
    }
    if ( _this != NULL ) {
        _EntityStatus(_this) = _Entity(datareader)->status;
    }       

    return _this;

}

void
_DataReaderViewFree (
    _DataReaderView dataReaderView)
{
    assert(dataReaderView);
    
    _EntityFreeStatusCondition(_Entity(dataReaderView));

    u_queryFree(dataReaderView->uQuery);
    u_dataViewFree(U_DATAREADERVIEW_GET(dataReaderView));
    

    gapi_setFree(dataReaderView->conditionSet);
    dataReaderView->conditionSet = NULL;

    gapi_loanRegistry_free(dataReaderView->loanRegistry);

    _DomainEntityDispose (_DomainEntity(dataReaderView));
}

gapi_boolean
_DataReaderViewPrepareDelete (
    _DataReaderView dataReaderView,
    gapi_context *context)
{
    gapi_boolean result = TRUE;

    assert(dataReaderView);

    if ( !gapi_setIsEmpty(dataReaderView->conditionSet) ) {
        gapi_errorReport(context, GAPI_ERRORCODE_CONTAINS_CONDITIONS);
        result = FALSE;
    }
    
    if ( !gapi_loanRegistry_is_empty(dataReaderView->loanRegistry) ) {
        gapi_errorReport(context, GAPI_ERRORCODE_CONTAINS_LOANS);
        result = FALSE;
    }


    return result;
}

u_dataView
_DataReaderViewUreaderView (
    _DataReaderView dataReaderView)
{
    return U_DATAREADERVIEW_GET(dataReaderView);
}

_DataReader
_DataReaderViewDataReader (
    _DataReaderView dataReaderView)
{
    _DataReader datareader;
 
    datareader = dataReaderView->datareader;
    _EntityClaim(datareader);   
    return datareader;
}

gapi_dataReaderViewQos *
_DataReaderViewGetQos (
    _DataReaderView dataReaderView,
    gapi_dataReaderViewQos * qos)
{
    v_dataViewQos dataViewQos;
    u_dataView uDataView;
    
    assert(dataReaderView);

    uDataView = u_dataView(U_DATAREADER_GET(dataReaderView));
        
    if ( u_entityQoS(u_entity(uDataView), (v_qos*)&dataViewQos) == U_RESULT_OK ) {
        copyReaderViewQosOut(dataViewQos,  qos);
        u_dataViewQosFree(dataViewQos);
    }

    return qos;
}

/*     ReturnCode_t
 *     set_qos(
 *         in DataReaderViewQos qos);
 *
 * Function will operate indepenedent of the enable flag
 */
gapi_returnCode_t
gapi_dataReaderView_set_qos (
    gapi_dataReaderView _this,
    const gapi_dataReaderViewQos *qos)
{
    gapi_returnCode_t result = GAPI_RETCODE_OK;
    u_result uResult;
    _DataReaderView dataReaderView;
    v_dataViewQos dataReaderViewQos;
    gapi_context context;

    GAPI_CONTEXT_SET(context, _this, GAPI_METHOD_SET_QOS);
    
    dataReaderView = gapi_dataReaderViewClaim(_this, &result);

    if (dataReaderView != NULL) {
        if ( dataReaderView && qos) {
           result = gapi_dataReaderViewQosIsConsistent(qos, &context);
        } else {
            result = GAPI_RETCODE_BAD_PARAMETER;
        }
        
        if ( result == GAPI_RETCODE_OK ) {
            gapi_dataReaderViewQos * existing_qos = gapi_dataReaderViewQos__alloc();

            result = gapi_dataReaderViewQosCheckMutability(qos, _DataReaderViewGetQos(dataReaderView, existing_qos), &context);
            gapi_free(existing_qos);
        }
    
        if ( result == GAPI_RETCODE_OK ) {
            dataReaderViewQos = u_dataViewQosNew(NULL);
            if (dataReaderViewQos) {
                if ( copyReaderViewQosIn(qos, dataReaderViewQos) ) {
                    uResult = u_entitySetQoS(_EntityUEntity(dataReaderView),(v_qos)(dataReaderViewQos) );
                    result = kernelResultToApiResult(uResult);
                } else {
                    result = GAPI_RETCODE_OUT_OF_RESOURCES;
                }
                u_dataViewQosFree(dataReaderViewQos);
            } else {
                result = GAPI_RETCODE_OUT_OF_RESOURCES;
            }
        }

        _EntityRelease(dataReaderView);
    }

    return result;
}

gapi_statusCondition
gapi_dataReaderView_get_statuscondition(
    gapi_dataReaderView _this)
{
    return GAPI_HANDLE_NIL;
}

gapi_statusMask
gapi_dataReaderView_get_status_changes(
    gapi_dataReaderView _this) 
{
    gapi_statusMask result = GAPI_STATUS_KIND_NULL;
    _DataReaderView dataReaderView;
    _DataReader dataReader;

    dataReaderView = gapi_dataReaderViewClaim(_this, NULL);

    if ( dataReaderView != NULL ) { 
        dataReader = _DataReaderViewDataReader(dataReaderView);

        if (dataReader != NULL) {
            result = _StatusGetCurrentStatus(_Entity(dataReader)->status);
            _EntityRelease(dataReader);
        }
 
        _EntityRelease(dataReaderView);
    }
    return result;
}

/*     void
 *     get_qos(
 *         inout DataReaderViewQos qos);
 *
 * Function will operate indepenedent of the enable flag
 */
gapi_returnCode_t
gapi_dataReaderView_get_qos (
    gapi_dataReaderView _this,
    gapi_dataReaderViewQos *qos)
{
    _DataReaderView dataReaderView;
    gapi_returnCode_t result;
    
    dataReaderView = gapi_dataReaderViewClaim(_this, &result);
    if ( dataReaderView && qos ) {
        _DataReaderViewGetQos(dataReaderView, qos);
    }

    _EntityRelease(dataReaderView);
    return result;
}

/*     DataReader
 *     get_datareader();
 */
gapi_dataReader
gapi_dataReaderView_get_datareader(
    gapi_dataReaderView _this)
{
    _DataReader datareader = NULL;
    _DataReaderView dataReaderView;

    dataReaderView = gapi_dataReaderViewClaim(_this, NULL);

    if ( dataReaderView != NULL ) { 
        datareader = _DataReaderViewDataReader(dataReaderView);
    }

    _EntityRelease(dataReaderView);
    
    return (gapi_dataReader)_EntityRelease(datareader);
}

/*     ReadCondition
 *     create_readcondition(
 *         in SampleStateMask sample_states,
 *         in ViewStateMask view_states,
 *         in InstanceStateMask instance_states);
 */
gapi_readCondition
gapi_dataReaderView_create_readcondition (
    gapi_dataReaderView _this,
    const gapi_sampleStateMask sample_states,
    const gapi_viewStateMask view_states,
    const gapi_instanceStateMask instance_states)
{
    _DataReaderView datareaderview;
    _ReadCondition readCondition = NULL;

    datareaderview = gapi_dataReaderViewClaim(_this, NULL);

    if ( datareaderview && _Entity(datareaderview)->enabled && 
         gapi_stateMasksValid(sample_states, view_states, instance_states) ) {

        _DataReader datareader;

        datareader = _DataReaderViewDataReader(datareaderview);
        readCondition = _ReadConditionNew ( sample_states,
                                            view_states,
                                            instance_states, 
                                            datareader,
                                            datareaderview);
        _EntityRelease(datareader);
        if ( readCondition != NULL ) {
            gapi_deleteEntityAction deleteAction;
            void *actionArg;
            
            gapi_setAdd(datareaderview->conditionSet,
                        (gapi_object)readCondition);

            if ( _ObjectGetDeleteAction(_Object(readCondition),
                                        &deleteAction, &actionArg) ) {
                _ObjectSetDeleteAction(_Object(readCondition),
                                       deleteAction,
                                       actionArg);
            }
            
            _ENTITY_REGISTER_OBJECT(_Entity(datareaderview),
                                    (_Object)readCondition);
        }
    }

    _EntityRelease(datareaderview);

    return (gapi_readCondition)_EntityRelease(readCondition);
}

/*     QueryCondition
 *     create_querycondition(
 *         in SampleStateMask sample_states,
 *         in ViewStateMask view_states,
 *         in InstanceStateMask instance_states,
 *         in string query_expression,
 *         in StringSeq query_parameters);
 */
gapi_queryCondition
gapi_dataReaderView_create_querycondition (
    gapi_dataReaderView _this,
    const gapi_sampleStateMask sample_states,
    const gapi_viewStateMask view_states,
    const gapi_instanceStateMask instance_states,
    const gapi_char *query_expression,
    const gapi_stringSeq *query_parameters)
{
    _DataReaderView datareaderview;
    _QueryCondition queryCondition = NULL;
    
    datareaderview = gapi_dataReaderViewClaim(_this, NULL);

    if ( datareaderview && _Entity(datareaderview)->enabled &&
         query_expression && gapi_sequence_is_valid(query_parameters) &&
         gapi_stateMasksValid(sample_states, view_states, instance_states) ) {

        _DataReader datareader;

        datareader = _DataReaderViewDataReader(datareaderview);
        queryCondition = _QueryConditionNew(sample_states,
                                            view_states,
                                            instance_states,
                                            query_expression,
                                            query_parameters,
                                            datareader,
                                            datareaderview);
        _EntityRelease(datareader);
        if ( queryCondition != NULL ) {
            gapi_setAdd(datareaderview->conditionSet,
                        (gapi_object)queryCondition);
            _ENTITY_REGISTER_OBJECT(_Entity(datareaderview),
                                    (_Object)queryCondition);
        }
    }

    _EntityRelease(datareaderview);
    
    return (gapi_queryCondition)_EntityRelease(queryCondition);
}


/*     ReturnCode_t
 *     delete_readcondition(
 *         in ReadCondition a_condition);
 */
gapi_returnCode_t
gapi_dataReaderView_delete_readcondition (
    gapi_dataReaderView _this,
    const gapi_readCondition a_condition)
{
    gapi_returnCode_t result = GAPI_RETCODE_OK;
    _DataReaderView datareaderview;
    _ReadCondition readCondition = NULL;

    datareaderview = gapi_dataReaderViewClaim(_this, &result);

    if ( datareaderview != NULL ) {
        readCondition = gapi_readConditionClaim(a_condition, NULL);
        if ( readCondition != NULL ) {
            gapi_setIter iterSet;

            iterSet = gapi_setFind (datareaderview->conditionSet,
                                    (gapi_object)readCondition);
            if ( gapi_setIterObject(iterSet) != NULL ) {
                gapi_setRemove(datareaderview->conditionSet,
                               (gapi_object)readCondition);
                _ReadConditionFree(readCondition);
            } else {
                gapi_setIterFree(iterSet);
            }
            gapi_setIterFree(iterSet);                
        } else {
            result = GAPI_RETCODE_BAD_PARAMETER;
        }
        _EntityRelease(datareaderview);
    }

    return result;
}


/*     ReturnCode_t
 *     delete_contained_entities();
 */
gapi_returnCode_t
gapi_dataReaderView_delete_contained_entities (
    gapi_dataReaderView _this,
    gapi_deleteEntityAction action,
    void *action_arg)
{
    gapi_returnCode_t result = GAPI_RETCODE_OK;
    _DataReaderView datareaderview;
    void *userData;

    datareaderview = gapi_dataReaderViewClaim(_this, &result);

    if ( datareaderview != NULL ) {
        gapi_setIter iterSet = gapi_setFirst(datareaderview->conditionSet);
        while ( gapi_setIterObject(iterSet) != NULL ) {
            _ReadCondition readCondition = (_ReadCondition)gapi_setIterObject(iterSet);
            _EntityClaim(readCondition);
            userData = _ObjectGetUserData(_Object(readCondition));
            _ReadConditionPrepareDelete(readCondition);
            _ReadConditionFree(readCondition);
            gapi_setIterRemove(iterSet);
            if ( action ) {
                action(userData, action_arg);
            }
        }
        gapi_setIterFree (iterSet);
        _EntityRelease(datareaderview);
    }

    return result;
}
