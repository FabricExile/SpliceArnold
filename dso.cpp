#include <ai.h>
#include <FabricSplice.h>
#include <vector>

class arnoldCritSec
{
public:
  arnoldCritSec()
  {
    if(sRefCount == 0)
      AiCritSecInitRecursive(&sSection);
    else
      lock();
    sRefCount++;
  }
  ~arnoldCritSec()
  {
    sRefCount--;
    if(sRefCount == 0)
    {
      AiCritSecClose(&sSection);
    }
    else
      unlock();
  }

  void lock() { AiCritSecEnter(&sSection); }
  void unlock() { AiCritSecLeave(&sSection); }

  static unsigned int sRefCount;
  static AtCritSec sSection;
};
unsigned int arnoldCritSec::sRefCount = 0;
AtCritSec arnoldCritSec::sSection = NULL;
arnoldCritSec sCritSec; 

struct userData {
  std::string fileName;
  FabricSplice::DGGraph graph;
  AtNode * procNode;
  std::vector<std::string> nodePortNames;
  std::vector<unsigned int> nodePortArrayId;
  std::vector<unsigned int> nodePortInstanceId;
  std::vector<AtNode*> nodePortShape;
  std::vector<FabricCore::RTVal> nodePortRTVal;
};

void arnoldLogFunc(const char * message, unsigned int length = 0)
{
  AiMsgInfo("[FabricSplice] %s\n", message);
}

void arnoldLogErrorFunc(const char * message, unsigned int length = 0)
{
  AiMsgWarning("[FabricSplice] %s\n", message);
}

void arnoldKLReportFunc(const char * message, unsigned int length)
{
  AiMsgInfo("[KL] %s\n", message);
}

void arnoldCompilerErrorFunc(unsigned int row, unsigned int col, const char * file, const char * level, const char * desc)
{
  AiMsgError("[KL Compiler %s]: file '%s', line %d: %s\n", level, file, (int)row, desc);
}

static int Init(AtNode *mynode, void **user_ptr)
{
  arnoldCritSec critSec;
  const char * fileName = (char*)AiNodeGetStr(mynode, "data");
  if(!fileName)
    return FALSE;
  AiMsgWarning("%s", fileName);

  userData * ud = new userData();
  ud->fileName = fileName;
  ud->procNode = mynode;

  try
  {
    ud->graph = FabricSplice::DGGraph("arnoldGraph");
    ud->graph.loadFromFile(ud->fileName.c_str());

    AtUserParamIterator * userParamIt = AiNodeGetUserParamIterator (ud->procNode);
    while(!AiUserParamIteratorFinished(userParamIt))
    {
      const AtUserParamEntry * userParam = AiUserParamIteratorGetNext(userParamIt);
      const char * paramName = AiUserParamGetName(userParam);

      FabricSplice::DGPort port = ud->graph.getDGPort(paramName);
      if(!port.isValid())
        continue;
      if(port.getMode() == FabricSplice::Port_Mode_OUT)
        continue;
      if(port.isArray())
        continue;

      std::string dataType = port.getDataType();

      if(AiUserParamGetType(userParam) == AI_TYPE_FLOAT && dataType == "Scalar")
        port.setVariant(FabricCore::Variant::CreateFloat64(AiNodeGetFlt(ud->procNode, paramName)));
      else if(AiUserParamGetType(userParam) == AI_TYPE_INT && dataType == "Integer")
        port.setVariant(FabricCore::Variant::CreateSInt32(AiNodeGetInt(ud->procNode, paramName)));
      else if(AiUserParamGetType(userParam) == AI_TYPE_STRING && dataType == "String")
        port.setVariant(FabricCore::Variant::CreateString(AiNodeGetStr(ud->procNode, paramName)));
    }

    ud->graph.evaluate();

    for(unsigned int i=0;i<ud->graph.getDGPortCount();i++)
    {
      std::string portName = ud->graph.getDGPortName(i);
      FabricSplice::DGPort port = ud->graph.getDGPort(portName.c_str());
      if(port.getMode() == FabricSplice::Port_Mode_IN)
        continue;
      std::string dataType = port.getDataType();
      if(dataType == "PolygonMesh")
      {
        FabricCore::RTVal val = port.getRTVal();
        if(!val.isValid())
          continue;
        if(val.isArray()) {
          for(unsigned int i=0;i<val.getArraySize();i++)
          {
            FabricCore::RTVal element = val.getArrayElement(i);
            if(element.isNullObject())
            {
              delete(ud);
              AiMsgError("Splice port '%s' contains a null object as an array element.", portName.c_str());
              return FALSE;
            }
            ud->nodePortNames.push_back(portName);
            ud->nodePortArrayId.push_back(i);
            ud->nodePortInstanceId.push_back(UINT_MAX);
            ud->nodePortRTVal.push_back(val);
          }
        }      
        else
        {
          if(val.isNullObject())
            continue;
          ud->nodePortNames.push_back(portName);
          ud->nodePortArrayId.push_back(UINT_MAX);
          ud->nodePortInstanceId.push_back(UINT_MAX);
          ud->nodePortRTVal.push_back(val);
        }
      }
      else if(dataType == "PolygonMeshInstancer")
      {
        FabricCore::RTVal val = port.getRTVal();
        if(!val.isValid())
          continue;
        if(val.isArray()) {
          for(unsigned int i=0;i<val.getArraySize();i++)
          {
            FabricCore::RTVal element = val.getArrayElement(i);
            if(element.isNullObject())
            {
              delete(ud);
              AiMsgError("Splice port '%s' contains a null object as an array element.", portName.c_str());
              return FALSE;
            }

            FabricCore::RTVal nbInstancesVal = element.callMethod("UInt32", "getNbInstances", 0, 0);
            unsigned int nbInstances = nbInstancesVal.getUInt32();

            // instances
            for(unsigned int j=0;j<nbInstances;j++)
            {
              ud->nodePortNames.push_back(portName);
              ud->nodePortArrayId.push_back(i);
              ud->nodePortInstanceId.push_back(j);
              ud->nodePortRTVal.push_back(val);
            }
          }
        }      
        else
        {
          if(val.isNullObject())
            continue;

          FabricCore::RTVal nbInstancesVal = val.callMethod("UInt32", "getNbInstances", 0, 0);
          unsigned int nbInstances = nbInstancesVal.getUInt32();

          // instances
          for(unsigned int j=0;j<nbInstances;j++)
          {
            ud->nodePortNames.push_back(portName);
            ud->nodePortArrayId.push_back(UINT_MAX);
            ud->nodePortInstanceId.push_back(j);
            ud->nodePortRTVal.push_back(val);
          }
        }
      }
    }
  }
  catch(FabricSplice::Exception e)
  {
    delete(ud);
    AiMsgError("%s", e.what());
    return FALSE;
  }
  catch(FabricCore::Exception e)
  {
    delete(ud);
    AiMsgError("%s", e.getDesc_cstr());
    return FALSE;
  }

  ud->nodePortShape.resize(ud->nodePortNames.size());
  for(size_t i=0;i<ud->nodePortShape.size();i++)
    ud->nodePortShape[i] = NULL;

  *user_ptr = ud;
  return TRUE;
}

// All done, deallocate stuff
static int Cleanup(void *user_ptr)
{
  arnoldCritSec critSec;
  userData * ud = (userData*)user_ptr;
  delete(ud);
  return TRUE;
}

// Get number of nodes
static int NumNodes(void *user_ptr)
{
  arnoldCritSec critSec;
  if(user_ptr == NULL)
    return 0;

  userData * ud = (userData*)user_ptr;
  return (int)ud->nodePortNames.size();
}

void shapeFromPolygonMesh(FabricCore::RTVal & val, AtNode *& node) {

  unsigned int nbPoints = val.callMethod("UInt64", "pointCount", 0, 0).getUInt64();
  unsigned int nbPolygons = val.callMethod("UInt64", "polygonCount", 0, 0).getUInt64();
  unsigned int nbSamples = val.callMethod("UInt64", "polygonPointsCount", 0, 0).getUInt64();

  AtArray * vlist = AiArrayAllocate(nbPoints, 1, AI_TYPE_POINT);
  AtArray * nsides = AiArrayAllocate(nbPolygons, 1, AI_TYPE_INT);
  AtArray * vidxs = AiArrayAllocate(nbSamples, 1, AI_TYPE_INT);
  AtArray * nlist = AiArrayAllocate(nbSamples, 1, AI_TYPE_VECTOR);

  if(nbPoints > 0)
  {
    std::vector<FabricCore::RTVal> args(2);
    args[0] = FabricSplice::constructExternalArrayRTVal("Float32", nbPoints * 3, vlist->data);
    args[1] = FabricSplice::constructUInt32RTVal(3); // components
    val.callMethod("", "getPointsAsExternalArray", 2, &args[0]);
  }
  if(nbSamples > 0)
  {
    std::vector<FabricCore::RTVal> args(1);
    args[0] = FabricSplice::constructExternalArrayRTVal("Float32", nbSamples * 3, nlist->data);
    val.callMethod("", "getNormalsAsExternalArray", 1, &args[0]);
  }
  if(nbPolygons > 0 && nbSamples > 0)
  {
    std::vector<FabricCore::RTVal> args(2);
    args[0] = FabricSplice::constructExternalArrayRTVal("UInt32", nbPolygons, nsides->data);
    args[1] = FabricSplice::constructExternalArrayRTVal("UInt32", nbSamples, vidxs->data);
    val.callMethod("", "getTopologyAsCountsIndicesExternalArrays", 2, &args[0]);
  }

  node = AiNode("polymesh");

  AtArray * nidxs = AiArrayAllocate(nbSamples, 1, AI_TYPE_INT);
  for(size_t i=0;i<nbSamples;i++)
    AiArraySetInt(nidxs, i, i);

  AiNodeSetArray(node, "vlist", vlist);
  AiNodeSetArray(node, "nsides", nsides);
  AiNodeSetArray(node, "vidxs", vidxs);
  AiNodeSetArray(node, "nlist", nlist);
  AiNodeSetArray(node, "nidxs", nidxs);
  AiNodeSetBool(node, "smoothing", true);
}

void shapeFromPolygonMeshInstancer(size_t nodeIdIn, FabricCore::RTVal & val, AtNode *& node, userData * ud, const std::string & portName, unsigned int arrayID, unsigned int instanceID) {
  size_t nodeId = nodeIdIn;

  if(instanceID != 0)
  {
    // find the master
    size_t masterId = nodeId - instanceID;
    if(ud->nodePortShape[masterId] == NULL)
    {
      // we need to instance, but we haven't hit the instancer master yet
      ud->nodePortInstanceId[masterId] = instanceID;
      ud->nodePortInstanceId[nodeId] = 0;
      shapeFromPolygonMeshInstancer(masterId, val, node, ud, portName, arrayID, 0);
      return;
    }

    if(node == NULL)
    {
      node = AiNode("ginstance");
      AiNodeSetPtr(node, "node", ud->nodePortShape[masterId]);
    }
  }
  else
  {
    // create the master shape!
    FabricCore::RTVal mesh = val.maybeGetMember("mesh");
    if(!mesh.isValid())
      return;
    if(mesh.isNullObject())
      return;
    shapeFromPolygonMesh(mesh, node);
  }

  // set the matrix
  if(node)
  {
    std::vector<FabricCore::RTVal> args(1);
    args[0] = FabricSplice::constructUInt32RTVal(instanceID); // components
    FabricCore::RTVal xfo = val.callMethod("Xfo", "getInstanceXfo", 1, &args[0]);
    FabricCore::RTVal mat44 = xfo.callMethod("Mat44", "toMat44", 0, NULL);
    FabricCore::RTVal row0 = mat44.maybeGetMember("row0");
    FabricCore::RTVal row1 = mat44.maybeGetMember("row1");
    FabricCore::RTVal row2 = mat44.maybeGetMember("row2");
    FabricCore::RTVal row3 = mat44.maybeGetMember("row3");

    AtMatrix m;
    m[0][0] = row0.maybeGetMember("x").getFloat32();
    m[1][0] = row0.maybeGetMember("y").getFloat32();
    m[2][0] = row0.maybeGetMember("z").getFloat32();
    m[3][0] = row0.maybeGetMember("t").getFloat32();
    m[0][1] = row1.maybeGetMember("x").getFloat32();
    m[1][1] = row1.maybeGetMember("y").getFloat32();
    m[2][1] = row1.maybeGetMember("z").getFloat32();
    m[3][1] = row1.maybeGetMember("t").getFloat32();
    m[0][2] = row2.maybeGetMember("x").getFloat32();
    m[1][2] = row2.maybeGetMember("y").getFloat32();
    m[2][2] = row2.maybeGetMember("z").getFloat32();
    m[3][2] = row2.maybeGetMember("t").getFloat32();
    m[0][3] = row3.maybeGetMember("x").getFloat32();
    m[1][3] = row3.maybeGetMember("y").getFloat32();
    m[2][3] = row3.maybeGetMember("z").getFloat32();
    m[3][3] = row3.maybeGetMember("t").getFloat32();

    AiNodeSetMatrix(node, "matrix", m);
    if(instanceID != 0)
      AiNodeSetBool(node, "inherit_xform", FALSE);
  }
}

unsigned int nbNodesCreated = 0;

// Get the i_th node
static AtNode *GetNode(void *user_ptr, int nodeIndex)
{
  arnoldCritSec critSec;

  userData * ud = (userData*)user_ptr;

  std::string nodePortName = ud->nodePortNames[nodeIndex];
  unsigned int nodeArrayIndex = ud->nodePortArrayId[nodeIndex];
  unsigned int nodeInstanceIndex = ud->nodePortInstanceId[nodeIndex];

  AtNode * shapeNode = NULL;
  try
  {
    FabricSplice::DGPort port = ud->graph.getDGPort(nodePortName.c_str());
    if(!port.isValid())
      return shapeNode;
    std::string dataType = port.getDataType();

    if(dataType == "PolygonMesh") {
      if(port.isArray())
      {
        FabricCore::RTVal arrayVal = ud->nodePortRTVal[nodeIndex];
        FabricCore::RTVal val = arrayVal.getArrayElement(nodeArrayIndex);
        shapeFromPolygonMesh(val, shapeNode);
      }
      else
      {
        FabricCore::RTVal val = ud->nodePortRTVal[nodeIndex];
        shapeFromPolygonMesh(val, shapeNode);
      }
    } else if(dataType == "PolygonMeshInstancer") {
      if(port.isArray())
      {
        FabricCore::RTVal arrayVal = ud->nodePortRTVal[nodeIndex];
        FabricCore::RTVal val = arrayVal.getArrayElement(nodeArrayIndex);
        shapeFromPolygonMeshInstancer(nodeIndex, val, shapeNode, ud, nodePortName, nodeArrayIndex, nodeInstanceIndex);
      }
      else
      {
        FabricCore::RTVal val = ud->nodePortRTVal[nodeIndex];
        shapeFromPolygonMeshInstancer(nodeIndex, val, shapeNode, ud, nodePortName, nodeArrayIndex, nodeInstanceIndex);
      }
    } else {
      AiMsgError("Unsupported Splice DataType '%s'.", dataType.c_str());
      return NULL;
    }
  }
  catch(FabricSplice::Exception e)
  {
    AiMsgError("%s", e.what());
    return NULL;
  }
  catch(FabricCore::Exception e)
  {
    AiMsgError("%s", e.getDesc_cstr());
    return NULL;
  }

  ud->nodePortShape[nodeIndex] = shapeNode;
  if(nbNodesCreated % 1000 == 0 || nbNodesCreated == ud->nodePortShape.size() - 1)
    AiMsgInfo("[SPLICE] %d out of %d elements created.", (int)nbNodesCreated, (int)ud->nodePortShape.size());
  nbNodesCreated++;
  return shapeNode;
}

// DSO hook
#ifdef __cplusplus
extern "C"
{
#endif

  AI_EXPORT_LIB int ProcLoader(AtProcVtable *vtable) 
  {
    vtable->Init     = Init;
    vtable->Cleanup  = Cleanup;
    vtable->NumNodes = NumNodes;
    vtable->GetNode  = GetNode;

    FabricSplice::Logging::setLogFunc(arnoldLogFunc);
    FabricSplice::Logging::setLogErrorFunc(arnoldLogErrorFunc);
    FabricSplice::Logging::setKLReportFunc(arnoldKLReportFunc);
    FabricSplice::Logging::setCompilerErrorFunc(arnoldCompilerErrorFunc);
    FabricSplice::Initialize();

    sprintf(vtable->version, AI_VERSION);
    return 1;
  }

#ifdef __cplusplus
}
#endif
