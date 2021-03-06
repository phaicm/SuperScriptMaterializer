#include "script_export.h"
//disable shit tip
#pragma warning(disable:26812)

#define changeSuffix(a) prefix[endIndex]='\0';strcat(prefix,a)
#define copyGuid(guid,str) sprintf(helper->_stringCache,"%d,%d",guid.d1,guid.d2);str=helper->_stringCache;
#define safeStringCopy(storage,str) storage=(str)?(str):"";

#pragma region inline func

void generate_pLink_in_pIn(CKContext* ctx, CKParameterIn* cache, scriptDatabase* db, dbScriptDataStructHelper* helper, EXPAND_CK_ID parents, EXPAND_CK_ID grandparents, int index, BOOL executedFromBB, BOOL isTarget) {
	//WARNING: i only choose one between [DirectSource] and [SharedSource] bucause i don't find any pIn both have these two field
	CKParameter* directSource = NULL;
	CKObject* ds_Owner = NULL;
	CKParameterIn* sharedSource = NULL;
	CKBehavior* ss_Owner = NULL;
	if (directSource = cache->GetDirectSource()) {
		helper->_db_pLink->input = directSource->GetID();
		if (directSource->GetClassID() == CKCID_PARAMETERLOCAL || directSource->GetClassID() == CKCID_PARAMETERVARIABLE) {
			//pLocal
			helper->_db_pLink->input_obj = directSource->GetID();
			helper->_db_pLink->input_type = pLinkInputOutputType_PLOCAL;
			helper->_db_pLink->input_is_bb = FALSE;
			helper->_db_pLink->input_index = -1;
		} else {
			//WARNING: when GetClassID() return CKDataArray there are untested code
			//pParam from bb pOut / pOper pOut / object Attribute / CKDataArray
			ds_Owner = directSource->GetOwner();
			switch (ds_Owner->GetClassID()) {
				case CKCID_BEHAVIOR:
					helper->_db_pLink->input_obj = ds_Owner->GetID();
					helper->_db_pLink->input_type = pLinkInputOutputType_POUT;
					helper->_db_pLink->input_is_bb = TRUE;
					helper->_db_pLink->input_index = ((CKBehavior*)ds_Owner)->GetOutputParameterPosition((CKParameterOut*)directSource);
					break;
				case CKCID_PARAMETEROPERATION:
					helper->_db_pLink->input_obj = ds_Owner->GetID();
					helper->_db_pLink->input_type = pLinkInputOutputType_POUT;
					helper->_db_pLink->input_is_bb = FALSE;
					helper->_db_pLink->input_index = 0;
					break;
				case CKCID_DATAARRAY:
					// dataarray, see as virtual bb pLocal shortcut
					helper->_db_pLink->input_obj = directSource->GetID();
					helper->_db_pLink->input_type = pLinkInputOutputType_PATTR;
					helper->_db_pLink->input_is_bb = FALSE;
					helper->_db_pLink->input_index = -1;
					proc_pAttr(ctx, db, helper, directSource);
					break;
				default:
					//normal object, see as virtual bb pLocal shortcut
					helper->_db_pLink->input_obj = directSource->GetID();
					helper->_db_pLink->input_type = pLinkInputOutputType_PATTR;
					helper->_db_pLink->input_is_bb = FALSE;
					helper->_db_pLink->input_index = -1;
					proc_pAttr(ctx, db, helper, directSource);
					break;
			}
		}
	}
	if (sharedSource = cache->GetSharedSource()) {
		//pIn from BB
		helper->_db_pLink->input = sharedSource->GetID();
		ss_Owner = (CKBehavior*)sharedSource->GetOwner();
		helper->_db_pLink->input_obj = ss_Owner->GetID();

		if (ss_Owner->IsUsingTarget() && (ss_Owner->GetTargetParameter() == sharedSource)) {
			//pTarget
			helper->_db_pLink->input_type = pLinkInputOutputType_PTARGET;
			helper->_db_pLink->input_is_bb = TRUE;
			helper->_db_pLink->input_index = -1;

		} else {
			//pIn
			helper->_db_pLink->input_type = pLinkInputOutputType_PIN;
			helper->_db_pLink->input_is_bb = TRUE;
			helper->_db_pLink->input_index = ss_Owner->GetInputParameterPosition(sharedSource);
		}


	}

	if (sharedSource != NULL || directSource != NULL) {
		helper->_db_pLink->output = cache->GetID();
		helper->_db_pLink->output_obj = parents;
		helper->_db_pLink->output_type = isTarget ? pLinkInputOutputType_PTARGET : pLinkInputOutputType_PIN;
		helper->_db_pLink->output_is_bb = executedFromBB;
		helper->_db_pLink->output_index = index;
		helper->_db_pLink->belong_to = grandparents;

		db->write_pLink(helper->_db_pLink);
	}
}

void proc_pTarget(CKContext* ctx, CKParameterIn* cache, scriptDatabase* db, dbScriptDataStructHelper* helper, EXPAND_CK_ID parents, EXPAND_CK_ID grandparents) {
	helper->_db_pTarget->thisobj = cache->GetID();
	helper->_db_pTarget->name = cache->GetName();
	helper->_db_pTarget->type = helper->_parameterManager->ParameterTypeToName(cache->GetType());
	copyGuid(cache->GetGUID(), helper->_db_pTarget->type_guid);
	helper->_db_pTarget->belong_to = parents;
	helper->_db_pTarget->direct_source = cache->GetDirectSource() ? cache->GetDirectSource()->GetID() : -1;
	helper->_db_pTarget->shared_source = cache->GetSharedSource() ? cache->GetSharedSource()->GetID() : -1;

	db->write_pTarget(helper->_db_pTarget);

	//judge whether expoer parameter and write database
	if (((CKBehavior*)ctx->GetObjectA(grandparents))->GetInputParameterPosition(cache) != -1) {
		helper->_db_eLink->export_obj = cache->GetID();
		helper->_db_eLink->internal_obj = parents;
		helper->_db_eLink->is_in = TRUE;
		helper->_db_eLink->index = -1;
		helper->_db_eLink->belong_to = grandparents;

		db->write_eLink(helper->_db_eLink);
		return;
	}

	//=========try generate pLink
	generate_pLink_in_pIn(ctx, cache, db, helper, parents, grandparents, -1, TRUE, TRUE);
}

void proc_pIn(CKContext* ctx, CKParameterIn* cache, scriptDatabase* db, dbScriptDataStructHelper* helper, EXPAND_CK_ID parents, EXPAND_CK_ID grandparents, int index, BOOL executedFromBB) {
	helper->_db_pIn->thisobj = cache->GetID();
	helper->_db_pIn->index = index;
	helper->_db_pIn->name = cache->GetName();
	CKParameterType vaildTypeChecker = cache->GetType();
	if (vaildTypeChecker != -1) helper->_db_pIn->type = helper->_parameterManager->ParameterTypeToName(cache->GetType()); //known types
	else helper->_db_pIn->type = "!!UNKNOW TYPE!!"; //unknow type
	copyGuid(cache->GetGUID(), helper->_db_pIn->type_guid);
	helper->_db_pIn->belong_to = parents;
	helper->_db_pIn->direct_source = cache->GetDirectSource() ? cache->GetDirectSource()->GetID() : -1;
	helper->_db_pIn->shared_source = cache->GetSharedSource() ? cache->GetSharedSource()->GetID() : -1;

	db->write_pIn(helper->_db_pIn);

	//judge whether expoer parameter and write database
	if (((CKBehavior*)ctx->GetObjectA(grandparents))->GetInputParameterPosition(cache) != -1) {
		helper->_db_eLink->export_obj = cache->GetID();
		helper->_db_eLink->internal_obj = parents;
		helper->_db_eLink->is_in = TRUE;
		helper->_db_eLink->index = index;
		helper->_db_eLink->belong_to = grandparents;

		db->write_eLink(helper->_db_eLink);
		return;
	}

	//=========try generate pLink
	generate_pLink_in_pIn(ctx, cache, db, helper, parents, grandparents, index, executedFromBB, FALSE);

}

void proc_pOut(CKContext* ctx, CKParameterOut* cache, scriptDatabase* db, dbScriptDataStructHelper* helper, EXPAND_CK_ID parents, EXPAND_CK_ID grandparents, int index, BOOL executedFromBB) {
	helper->_db_pOut->thisobj = cache->GetID();
	helper->_db_pOut->index = index;
	helper->_db_pOut->name = cache->GetName();
	CKParameterType vaildTypeChecker = cache->GetType();
	if (vaildTypeChecker != -1) helper->_db_pOut->type = helper->_parameterManager->ParameterTypeToName(cache->GetType()); //known types
	else helper->_db_pOut->type = "!!UNKNOW TYPE!!"; //unknow type
	copyGuid(cache->GetGUID(), helper->_db_pOut->type_guid);
	helper->_db_pOut->belong_to = parents;

	db->write_pOut(helper->_db_pOut);

	//judge whether expoer parameter and write database
	if (((CKBehavior*)ctx->GetObjectA(grandparents))->GetOutputParameterPosition(cache) != -1) {
		helper->_db_eLink->export_obj = cache->GetID();
		helper->_db_eLink->internal_obj = parents;
		helper->_db_eLink->is_in = FALSE;
		helper->_db_eLink->index = index;
		helper->_db_eLink->belong_to = grandparents;

		db->write_eLink(helper->_db_eLink);
		return;
	}

	//=========try generate pLink
	CKParameter* cache_Dest = NULL;
	CKObject* cache_DestOwner = NULL;
	for (int j = 0, jCount = cache->GetDestinationCount(); j < jCount; j++) {
		cache_Dest = cache->GetDestination(j);

		helper->_db_pLink->input = cache->GetID();
		helper->_db_pLink->input_obj = parents;
		helper->_db_pLink->input_type = pLinkInputOutputType_POUT;
		helper->_db_pLink->input_is_bb = executedFromBB;
		helper->_db_pLink->input_index = index;

		helper->_db_pLink->output = cache_Dest->GetID();
		if (cache_Dest->GetClassID() == CKCID_PARAMETERLOCAL) {
			//pLocal
			helper->_db_pLink->output_obj = cache_Dest->GetID();
			helper->_db_pLink->output_type = pLinkInputOutputType_PLOCAL;
			helper->_db_pLink->output_is_bb = FALSE;
			helper->_db_pLink->output_index = -1;

		} else {
			//pOut, it must belong to a BB

			cache_DestOwner = cache_Dest->GetOwner();
			helper->_db_pLink->output_obj = cache_DestOwner->GetID();
			helper->_db_pLink->output_type = pLinkInputOutputType_POUT;
			helper->_db_pLink->output_is_bb = TRUE;
			helper->_db_pLink->output_index = ((CKBehavior*)cache_DestOwner)->GetOutputParameterPosition((CKParameterOut*)cache_Dest);

		}

		helper->_db_pLink->belong_to = grandparents;

		db->write_pLink(helper->_db_pLink);
	}
}

void proc_bIn(CKBehaviorIO* cache, scriptDatabase* db, dbScriptDataStructHelper* helper, EXPAND_CK_ID parents, int index) {
	helper->_db_bIn->thisobj = cache->GetID();
	helper->_db_bIn->index = index;
	helper->_db_bIn->name = cache->GetName();
	helper->_db_bIn->belong_to = parents;

	db->write_bIn(helper->_db_bIn);
}

void proc_bOut(CKBehaviorIO* cache, scriptDatabase* db, dbScriptDataStructHelper* helper, EXPAND_CK_ID parents, int index) {
	helper->_db_bOut->thisobj = cache->GetID();
	helper->_db_bOut->index = index;
	helper->_db_bOut->name = cache->GetName();
	helper->_db_bOut->belong_to = parents;

	db->write_bOut(helper->_db_bOut);
}

void proc_bLink(CKBehaviorLink* cache, scriptDatabase* db, dbScriptDataStructHelper* helper, EXPAND_CK_ID parents) {
	CKBehaviorIO* io = cache->GetInBehaviorIO();
	CKBehavior* beh = io->GetOwner();
	helper->_db_bLink->input = io->GetID();
	helper->_db_bLink->input_obj = beh->GetID();
	helper->_db_bLink->input_type = (io->GetType() == CK_BEHAVIORIO_IN ? bLinkInputOutputType_INPUT : bLinkInputOutputType_OUTPUT);
	helper->_db_bLink->input_index = (io->GetType() == CK_BEHAVIORIO_IN ? io->GetOwner()->GetInputPosition(io) : io->GetOwner()->GetOutputPosition(io));
	io = cache->GetOutBehaviorIO();
	beh = io->GetOwner();
	helper->_db_bLink->output = io->GetID();
	helper->_db_bLink->output_obj = beh->GetID();
	helper->_db_bLink->output_type = (io->GetType() == CK_BEHAVIORIO_IN ? bLinkInputOutputType_INPUT : bLinkInputOutputType_OUTPUT);
	helper->_db_bLink->output_index = (io->GetType() == CK_BEHAVIORIO_IN ? io->GetOwner()->GetInputPosition(io) : io->GetOwner()->GetOutputPosition(io));

	helper->_db_bLink->delay = cache->GetActivationDelay();
	helper->_db_bLink->belong_to = parents;

	db->write_bLink(helper->_db_bLink);
}

void proc_pLocal(CKParameterLocal* cache, scriptDatabase* db, dbScriptDataStructHelper* helper, EXPAND_CK_ID parents, BOOL is_setting) {
	helper->_db_pLocal->thisobj = cache->GetID();
	helper->_db_pLocal->name = cache->GetName() ? cache->GetName() : "";
	CKParameterType vaildTypeChecker = cache->GetType();
	if (vaildTypeChecker != -1) helper->_db_pLocal->type = helper->_parameterManager->ParameterTypeToName(cache->GetType()); //known types
	else helper->_db_pLocal->type = "!!UNKNOW TYPE!!"; //unknow type
	copyGuid(cache->GetGUID(), helper->_db_pLocal->type_guid);
	helper->_db_pLocal->is_setting = is_setting;
	helper->_db_pLocal->belong_to = parents;

	db->write_pLocal(helper->_db_pLocal);

	//export plocal metadata
	DigParameterData(cache, db, helper, cache->GetID());
}

void proc_pOper(CKContext* ctx, CKParameterOperation* cache, scriptDatabase* db, dbScriptDataStructHelper* helper, EXPAND_CK_ID parents) {
	helper->_db_pOper->thisobj = cache->GetID();
	helper->_db_pOper->op = helper->_parameterManager->OperationGuidToName(cache->GetOperationGuid());
	copyGuid(cache->GetOperationGuid(), helper->_db_pOper->op_guid);
	helper->_db_pOper->belong_to = parents;

	db->write_pOper(helper->_db_pOper);

	//export 2 input param and 1 output param
	proc_pIn(ctx, cache->GetInParameter1(), db, helper, cache->GetID(), parents, 0, FALSE);
	proc_pIn(ctx, cache->GetInParameter2(), db, helper, cache->GetID(), parents, 1, FALSE);
	proc_pOut(ctx, cache->GetOutParameter(), db, helper, cache->GetID(), parents, 0, FALSE);
}

void proc_pAttr(CKContext* ctx, scriptDatabase* db, dbScriptDataStructHelper* helper, CKParameter* cache) {
	//write self first to detect conflict
	helper->_db_pAttr->thisobj = cache->GetID();
	safeStringCopy(helper->_db_pAttr->name, cache->GetName());
	CKParameterType vaildTypeChecker = cache->GetType();
	if (vaildTypeChecker != -1) helper->_db_pAttr->type = helper->_parameterManager->ParameterTypeToName(cache->GetType()); //known types
	else helper->_db_pAttr->type = "!!UNKNOW TYPE!!"; //unknow type
	copyGuid(cache->GetGUID(), helper->_db_pAttr->type_guid);

	if (!db->write_pAttr(helper->_db_pAttr))
		return;

	//not duplicated, continue write property
	CKObject* host = cache->GetOwner();
	helper_pDataExport("attr.host_id", (long)host->GetID(), db, helper, cache->GetID());
	helper_pDataExport("attr.host_name", host->GetName(), db, helper, cache->GetID());
}

//============================helper for pLocal data export
void helper_pDataExport(const char* field, const char* data, scriptDatabase* db, dbScriptDataStructHelper* helper, EXPAND_CK_ID parents) {
	helper->_db_pData->field = field;
	helper->_db_pData->data = data;
	helper->_db_pData->belong_to = parents;

	db->write_pData(helper->_db_pData);
}
void helper_pDataExport(const char* field, float data, scriptDatabase* db, dbScriptDataStructHelper* helper, EXPAND_CK_ID parents) {
	char str[32];
	sprintf(str, "%f", data);
	helper_pDataExport(field, str, db, helper, parents);
}
void helper_pDataExport(const char* field, long data, scriptDatabase* db, dbScriptDataStructHelper* helper, EXPAND_CK_ID parents) {
	char str[32];
	ltoa(data, str, 10);
	helper_pDataExport(field, str, db, helper, parents);
}


#pragma endregion


#pragma region normal func

void IterateScript(CKContext* ctx, scriptDatabase* db, dbScriptDataStructHelper* helper) {
	CKBeObject* beobj = NULL;
	CKBehavior* beh = NULL;
	XObjectPointerArray objArray = ctx->GetObjectListByType(CKCID_BEOBJECT, TRUE);
	int len = objArray.Size();
	int scriptLen = 0;
	for (int i = 0; i < len; i++) {
		beobj = (CKBeObject*)objArray.GetObjectA(i);
		if ((scriptLen = beobj->GetScriptCount()) == 0) continue;
		for (int j = 0; j < scriptLen; j++) {
			//write script table
			beh = beobj->GetScript(j);

			helper->_dbCKScript->thisobj = beobj->GetID();
			helper->_dbCKScript->host_name = beobj->GetName();
			helper->_dbCKScript->index = j;
			helper->_dbCKScript->behavior = beh->GetID();
			db->write_CKScript(helper->_dbCKScript);

			//iterate script
			IterateBehavior(ctx, beh, db, helper, -1);
		}
	}
}

void IterateBehavior(CKContext* ctx, CKBehavior* bhv, scriptDatabase* db, dbScriptDataStructHelper* helper, EXPAND_CK_ID parents) {
	//write self data
	helper->_dbCKBehavior->thisobj = bhv->GetID();
	helper->_dbCKBehavior->name = bhv->GetName();
	helper->_dbCKBehavior->type = bhv->GetType();
	helper->_dbCKBehavior->proto_name = bhv->GetPrototypeName() ? bhv->GetPrototypeName() : "";
	copyGuid(bhv->GetPrototypeGuid(), helper->_dbCKBehavior->proto_guid);
	helper->_dbCKBehavior->flags = bhv->GetFlags();
	helper->_dbCKBehavior->priority = bhv->GetPriority();
	helper->_dbCKBehavior->version = bhv->GetVersion();
	helper->_dbCKBehavior->parent = parents;
	sprintf(helper->_stringCache, "%d,%d,%d,%d,%d",
		(bhv->IsUsingTarget() ? 1 : 0),
		bhv->GetInputParameterCount(),
		bhv->GetOutputParameterCount(),
		bhv->GetInputCount(),
		bhv->GetOutputCount());
	helper->_dbCKBehavior->pin_count = helper->_stringCache;
	db->write_CKBehavior(helper->_dbCKBehavior);

	//write target
	if (bhv->IsUsingTarget())
		proc_pTarget(ctx, bhv->GetTargetParameter(), db, helper, bhv->GetID(), parents);

	int count = 0, i = 0;
	//pIn
	for (i = 0, count = bhv->GetInputParameterCount(); i < count; i++)
		proc_pIn(ctx, bhv->GetInputParameter(i), db, helper, bhv->GetID(), parents, i, TRUE);
	//pOut
	for (i = 0, count = bhv->GetOutputParameterCount(); i < count; i++)
		proc_pOut(ctx, bhv->GetOutputParameter(i), db, helper, bhv->GetID(), parents, i, TRUE);
	//bIn
	for (i = 0, count = bhv->GetInputCount(); i < count; i++)
		proc_bIn(bhv->GetInput(i), db, helper, bhv->GetID(), i);
	//bOut
	for (i = 0, count = bhv->GetOutputCount(); i < count; i++)
		proc_bOut(bhv->GetOutput(i), db, helper, bhv->GetID(), i);
	//bLink
	for (i = 0, count = bhv->GetSubBehaviorLinkCount(); i < count; i++)
		proc_bLink(bhv->GetSubBehaviorLink(i), db, helper, bhv->GetID());
	//pLocal
	for (i = 0, count = bhv->GetLocalParameterCount(); i < count; i++)
		proc_pLocal(bhv->GetLocalParameter(i), db, helper, bhv->GetID(),
			bhv->IsLocalParameterSetting(i));
	//pOper
	for (i = 0, count = bhv->GetParameterOperationCount(); i < count; i++)
		proc_pOper(ctx, bhv->GetParameterOperation(i), db, helper, bhv->GetID());

	//iterate sub bb
	for (i = 0, count = bhv->GetSubBehaviorCount(); i < count; i++)
		IterateBehavior(ctx, bhv->GetSubBehavior(i), db, helper, bhv->GetID());
}

void DigParameterData(CKParameterLocal* p, scriptDatabase* db, dbScriptDataStructHelper* helper, EXPAND_CK_ID parents) {
	CKGUID t = p->GetGUID();
	BOOL unknowType = FALSE;

	if (t.d1 & t.d2) unknowType = TRUE;

	//nothing
	if (t == CKPGUID_NONE) return;
	if (p->GetParameterClassID() && p->GetValueObject(false)) {
		helper_pDataExport("id", (long)p->GetValueObject(false)->GetID(), db, helper, parents);
		helper_pDataExport("name", p->GetValueObject(false)->GetName(), db, helper, parents);
		helper_pDataExport("type", p->GetValueObject(false)->GetClassNameA(), db, helper, parents);
		return;
	}
	//float
	if (t == CKPGUID_FLOAT || t == CKPGUID_ANGLE || t == CKPGUID_PERCENTAGE || t == CKPGUID_TIME
		|| t == CKPGUID_FLOATSLIDER) {
		helper_pDataExport("float-data", *(float*)(p->GetReadDataPtr(false)), db, helper, parents);
		return;
	}
	//int
	if (t == CKPGUID_INT || t == CKPGUID_KEY || t == CKPGUID_BOOL || t == CKPGUID_ID || t == CKPGUID_POINTER
		|| t == CKPGUID_MESSAGE || t == CKPGUID_ATTRIBUTE || t == CKPGUID_BLENDMODE || t == CKPGUID_FILTERMODE
		|| t == CKPGUID_BLENDFACTOR || t == CKPGUID_FILLMODE || t == CKPGUID_LITMODE || t == CKPGUID_SHADEMODE
		|| t == CKPGUID_ADDRESSMODE || t == CKPGUID_WRAPMODE || t == CKPGUID_3DSPRITEMODE || t == CKPGUID_FOGMODE
		|| t == CKPGUID_LIGHTTYPE || t == CKPGUID_SPRITEALIGN || t == CKPGUID_DIRECTION || t == CKPGUID_LAYERTYPE
		|| t == CKPGUID_COMPOPERATOR || t == CKPGUID_BINARYOPERATOR || t == CKPGUID_SETOPERATOR
		|| t == CKPGUID_OBSTACLEPRECISION || t == CKPGUID_OBSTACLEPRECISIONBEH) {
		helper_pDataExport("int-data", (long)(*(int*)(p->GetReadDataPtr(false))), db, helper, parents);
		return;
	}
	if (t == CKPGUID_VECTOR) {
		VxVector vec;
		memcpy(&vec, p->GetReadDataPtr(false), sizeof(vec));
		helper_pDataExport("vector.x", vec.x, db, helper, parents);
		helper_pDataExport("vector.y", vec.y, db, helper, parents);
		helper_pDataExport("vector.z", vec.z, db, helper, parents);
		return;
	}
	if (t == CKPGUID_2DVECTOR) {
		Vx2DVector vec;
		memcpy(&vec, p->GetReadDataPtr(false), sizeof(vec));
		helper_pDataExport("2dvector.x", vec.x, db, helper, parents);
		helper_pDataExport("2dvector.y", vec.y, db, helper, parents);
		return;
	}
	if (t == CKPGUID_MATRIX) {
		VxMatrix mat;
		char position[128];
		memcpy(&mat, p->GetReadDataPtr(false), sizeof(mat));

		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				sprintf(position, "matrix[%d][%d]", i, j);
				helper_pDataExport(position, mat[i][j], db, helper, parents);
			}
		}
		return;
	}
	if (t == CKPGUID_COLOR) {
		VxColor col;
		memcpy(&col, p->GetReadDataPtr(false), sizeof(col));
		helper_pDataExport("color.r", col.r, db, helper, parents);
		helper_pDataExport("color.g", col.g, db, helper, parents);
		helper_pDataExport("color.b", col.b, db, helper, parents);
		helper_pDataExport("color.a", col.a, db, helper, parents);
		return;
	}
	if (t == CKPGUID_2DCURVE) {
		CK2dCurve* c;
		char prefix[128];
		int endIndex = 0;
		memcpy(&c, p->GetReadDataPtr(false), sizeof(c));
		for (int i = 0, cc = c->GetControlPointCount(); i < cc; ++i) {
			sprintf(prefix, "2dcurve.control_point[%d]", i);
			endIndex = strlen(prefix);

			changeSuffix(".pos.x");
			helper_pDataExport(prefix, c->GetControlPoint(i)->GetPosition().x, db, helper, parents);
			changeSuffix(".pos.y");
			helper_pDataExport(prefix, c->GetControlPoint(i)->GetPosition().y, db, helper, parents);
			changeSuffix(".islinear");
			helper_pDataExport(prefix, (long)c->GetControlPoint(i)->IsLinear(), db, helper, parents);
			if (c->GetControlPoint(i)->IsTCB()) {
				changeSuffix(".bias");
				helper_pDataExport(prefix, c->GetControlPoint(i)->GetBias(), db, helper, parents);
				changeSuffix(".continuity");
				helper_pDataExport(prefix, c->GetControlPoint(i)->GetContinuity(), db, helper, parents);
				changeSuffix(".tension");
				helper_pDataExport(prefix, c->GetControlPoint(i)->GetTension(), db, helper, parents);
			} else {
				changeSuffix(".intangent.x");
				helper_pDataExport(prefix, c->GetControlPoint(i)->GetInTangent().x, db, helper, parents);
				changeSuffix(".intangent.y");
				helper_pDataExport(prefix, c->GetControlPoint(i)->GetInTangent().y, db, helper, parents);
				changeSuffix(".outtangent.x");
				helper_pDataExport(prefix, c->GetControlPoint(i)->GetOutTangent().x, db, helper, parents);
				changeSuffix(".outtangent.y");
				helper_pDataExport(prefix, c->GetControlPoint(i)->GetOutTangent().y, db, helper, parents);
			}
		}
		return;
	}
	if (t == CKPGUID_STRING) {
		char* cptr = (char*)p->GetReadDataPtr(false);
		int cc = p->GetDataSize();

		helper->_db_pData->data.clear();
		helper->_db_pData->data.insert(0, cptr, 0, cc);
		helper->_db_pData->field = "str";
		helper->_db_pData->belong_to = p->GetID();
		db->write_pData(helper->_db_pData);
		return;
	}

	unknowType = TRUE;
	//if it gets here, we have no idea what it really is. so simply dump it.
	//buffer-like
	if (unknowType || t == CKPGUID_VOIDBUF || t == CKPGUID_SHADER || t == CKPGUID_TECHNIQUE || t == CKPGUID_PASS) {
		//dump data
		unsigned char* cptr = (unsigned char*)p->GetReadDataPtr(false);
		char temp[8];
		int cc = 0, rcc = 0, pos = 0;
		rcc = cc = p->GetDataSize();
		if (rcc > 1024) rcc = 1024;

		helper->_db_pData->data.clear();
		for (int i = 0; i < rcc; i++) {
			sprintf(temp, "0x%02X", cptr[i]);

			helper->_db_pData->data += temp;
			if (i != rcc - 1)
				helper->_db_pData->data += ", ";
		}

		if (rcc == cc)
			helper->_db_pData->field = "dump.data";
		else
			helper->_db_pData->field = "dump.partial_data";
		helper->_db_pData->belong_to = p->GetID();
		db->write_pData(helper->_db_pData);

		//dump data length
		helper_pDataExport("dump.length", (long)cc, db, helper, parents);
		return;
	}
}

#pragma endregion

