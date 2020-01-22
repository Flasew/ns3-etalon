#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/csma-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/multichannel-probe-module.h"
#include "ns3/random-variable-stream.h"

#include <unordered_map>
#include <ctime>
#include <iomanip>
#include <random>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TCP_HTTP");

struct LinkParam {
  uint64_t bandwidth;
  std::string delay_s;
  uint64_t period;
  Time delay;
  bool direct;
  LinkParam(uint64_t b, std::string d, uint64_t p, bool c): bandwidth(b), delay_s(d), period(p), delay(d), direct(c){}
};

class TopoHelper {

public:
  static NodeContainer allNodes;
  static std::unordered_map<uint64_t, NetDeviceContainer> netList;
  static std::unordered_map<uint64_t, Ipv4InterfaceContainer> ifList;
  
  static std::normal_distribution<double> rjitnd;
  static Ptr<NormalRandomVariable> hopnd;

  static void 
  Init(uint32_t n) {
    allNodes.Create(n);
  }

  static uint64_t 
  BuildIndex(uint32_t i, uint32_t j) {
    if (i > j) {
      uint32_t tmp = i;
      i = j;
      j = tmp;
    }
    uint64_t result = i;
    result <<= 32;
    result |= j;
    return result;
  }

  static NetDeviceContainer & 
  GetLink(uint32_t i, uint32_t j) {
    return netList[TopoHelper::BuildIndex(i, j)];
  }

  static Ipv4InterfaceContainer & 
  GetIf(uint32_t i, uint32_t j) {
    return ifList[TopoHelper::BuildIndex(i, j)];
  }

  static NodeContainer 
  GetContainerOf(std::initializer_list<uint32_t> args) {

    NodeContainer r;
    for (auto i: args) {
      r.Add(allNodes.Get(i));
    }
    return r;
  }

  static NodeContainer 
  Connect(PointToPointHelper & p2p, uint32_t i, uint32_t j) {

    NodeContainer n = TopoHelper::GetContainerOf({i, j});
    netList[TopoHelper::BuildIndex(i, j)] = p2p.Install(n);

    return n; 
  }

  static void 
  Connect(PointToPointHelper & p2p, 
    NodeContainer & nodes,
    uint32_t i, 
    uint32_t j) { 

    netList[TopoHelper::BuildIndex(i, j)] = p2p.Install(nodes);

  }

  static void
  AssignIP(Ipv4AddressHelper & iphelper, uint32_t i, uint32_t j) {

    ifList[TopoHelper::BuildIndex(i, j)] = 
    iphelper.Assign(netList[TopoHelper::BuildIndex(i, j)]);

  }

  static void 
  ChangeBW(LinkParam & l, uint32_t i, uint32_t j, std::default_random_engine & gen, bool ratejitter=true) {

    uint64_t rate = l.bandwidth;
    if (ratejitter) {
      // int jit = rjitnd(gen);
      // if (-(jit*2) > 0 && (uint64_t)(-(jit*2)) >= rate)
      //   jit = 0;
      rate = rate + rjitnd(gen);
    }

    NS_LOG_INFO("Rate changing to " << rate);

    auto d = TopoHelper::GetLink(i, j);
    auto dev_p = d.Get(0);
    auto dev = DynamicCast<PointToPointNetDevice>(dev_p);
    dev->SetDataRate(DataRate(rate));
    //dev->SetDelay(delay);
    Time delay = l.delay;
    if (!l.direct) {
      delay = delay * hopnd->GetInteger();
    }
    (DynamicCast<PointToPointChannel>(dev->GetChannel()))->SetDelay(delay);
    dev_p = d.Get(1);
    dev = DynamicCast<PointToPointNetDevice>(dev_p);
    dev->SetDataRate(DataRate(rate));
    //dev->SetDelay(delay);
    //(DynamicCast<PointToPointChannel>(dev->GetChannel()))->SetDelay(delay);

  }

  static void
  LinkUp(uint32_t i, uint32_t j) {

    auto if0 = ifList[TopoHelper::BuildIndex(i, j)].Get(0);
    if0.first->GetObject<Ipv4L3Protocol>()->GetInterface(if0.second)->SetUp();
    auto if1 = ifList[TopoHelper::BuildIndex(i, j)].Get(1);
    if1.first->GetObject<Ipv4L3Protocol>()->GetInterface(if1.second)->SetUp();

  }

  static void
  LinkDown(uint32_t i, uint32_t j) {

    auto if0 = ifList[TopoHelper::BuildIndex(i, j)].Get(0);
    if0.first->GetObject<Ipv4L3Protocol>()->GetInterface(if0.second)->SetDown();
    auto if1 = ifList[TopoHelper::BuildIndex(i, j)].Get(1);
    if1.first->GetObject<Ipv4L3Protocol>()->GetInterface(if1.second)->SetDown();

  }

};

NodeContainer TopoHelper::allNodes = NodeContainer();
std::unordered_map<uint64_t, NetDeviceContainer> TopoHelper::netList = std::unordered_map<uint64_t, NetDeviceContainer>();
std::unordered_map<uint64_t, Ipv4InterfaceContainer> TopoHelper::ifList = std::unordered_map<uint64_t, Ipv4InterfaceContainer>();

std::normal_distribution<double> TopoHelper::rjitnd = std::normal_distribution<double>();
Ptr<NormalRandomVariable> TopoHelper::hopnd = CreateObject<NormalRandomVariable> ();

std::normal_distribution<double> tjitnd;

std::ofstream qllog;
std::ofstream frlog;
std::ofstream lolog;
std::ofstream httplatlog;
std::ofstream cwndlog;
std::ofstream config;

std::string bwt_str;
std::string hdelay = "10us";
std::string l_inter = "100us";
std::vector<LinkParam> bwt;

double nsd = 5000.0;
double rjitter = 100000.0;
double tjitter = 20.0;
double hopm = 3.5;
double hops = 1;
double hopb = 2;
uint64_t simtime = 30;
uint64_t sendtime = 20;
uint64_t h_rate = 40000000000;
uint64_t rwnd = 262144;
uint64_t nflows = 1;
uint64_t q_size = 60;
uint64_t objszmin = 512;
uint64_t objszmax = 512000;
uint64_t objszmean = 100000;
uint64_t objszsd = 25000;
uint64_t reqsz = 500;
bool nochange = false;
bool indivlog = true;
bool bidir = false;
bool rjitter_enable = true;
bool tjitter_enable = true;

std::default_random_engine  gen;

int curr_rate = 0;

void CycleRate() {

  curr_rate = (curr_rate + 1) % bwt.size();
  TopoHelper::ChangeBW(bwt[curr_rate], 2, 3, gen, rjitter_enable);

  uint64_t t = bwt[curr_rate].period;

  if (tjitter_enable) {
    int jit = tjitnd(gen);
    if ((jit) < 0 && (uint64_t)(-(jit*2)) >= t)
      jit = 0;
    t = t + jit;
  }

  Simulator::Schedule(MicroSeconds(t), CycleRate);
}

// void QlTrace (std::string ctxt, uint32_t oldValue, uint32_t newValue)
// {

//   if (qllog.is_open()) {
//     if (ctxt[10] == '2')
//       qllog << Simulator::Now().GetNanoSeconds() << ", 2, " << newValue << std::endl;
//     else if (ctxt[10] == '3')
//       qllog << Simulator::Now().GetNanoSeconds() << ", 3, " << newValue << std::endl;
//   }

// }

// static void CwndTrace (std::string ctxt, uint32_t oldValue, uint32_t newValue)
// {
//   if (cwndlog.is_open()) 
//     cwndlog <<  Simulator::Now().GetNanoSeconds() << ", " << ctxt << ", " << newValue << std::endl;
// }

// std::vector<uint64_t> frdata;
// std::vector<uint64_t> lodata;

// static void
// CongStateTrace (std::string ctxt, 
//   const TcpSocketState::TcpCongState_t oldValue, 
//   const TcpSocketState::TcpCongState_t newValue)
// {
//   uint64_t flowid = stoi(ctxt);

//   if (oldValue != TcpSocketState::CA_RECOVERY && newValue == TcpSocketState::CA_RECOVERY) {
//     ++frdata[flowid]; 
//     if (frlog.is_open())
//       frlog <<  Simulator::Now().GetNanoSeconds() << ", " << ctxt << ", " << frdata[flowid] << std::endl;
//   }

//   if (oldValue != TcpSocketState::CA_LOSS && newValue == TcpSocketState::CA_LOSS) {
//     ++lodata[flowid];
//     if (lolog.is_open())
//       lolog <<  Simulator::Now().GetNanoSeconds() << ", " << ctxt << ", " << lodata[flowid] << std::endl;
//   }

// }

// static void
// TcpStateTrace (std::string ctxt, 
//   const TcpSocket::TcpStates_t oldValue, 
//   const TcpSocket::TcpStates_t newValue)
// {
//   return;
// }

// std::vector<uint64_t> syndata;

// static void TxPktTrace (std::string ctxt,  
//   const Ptr< const Packet > packet, 
//   const TcpHeader &header, 
//   const Ptr< const TcpSocketBase > socket)
// {
//   if (header.GetFlags() & TcpHeader::SYN)
//     ++syndata[stoi(ctxt)];
// }

uint64_t mainObjSuccess = 0;
uint64_t mainObjBad = 0;

uint64_t embObjSuccess = 0;
uint64_t embObjBad = 0;

void
ClientMainObjectReceived (Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet)
{
  Ptr<Packet> p = packet->Copy ();
  ThreeGppHttpHeader header;
  p->RemoveHeader (header);
  if (header.GetContentLength () == p->GetSize ()
      && header.GetContentType () == ThreeGppHttpHeader::MAIN_OBJECT)
    {
      NS_LOG_INFO ("Client has successfully received a main object of "
                   << p->GetSize () << " bytes.");
      mainObjSuccess++;
    }
  else
    {
      NS_LOG_INFO ("Client failed to parse a main object. ");
      mainObjBad++;
    }
}

void
ClientEmbeddedObjectReceived (Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet)
{
  Ptr<Packet> p = packet->Copy ();
  ThreeGppHttpHeader header;
  p->RemoveHeader (header);
  if (header.GetContentLength () == p->GetSize ()
      && header.GetContentType () == ThreeGppHttpHeader::EMBEDDED_OBJECT)
    {
      NS_LOG_INFO ("Client has successfully received an embedded object of "
                   << p->GetSize () << " bytes.");
      embObjSuccess++;
    }
  else
    {
      NS_LOG_INFO ("Client failed to parse an embedded object. ");
      embObjBad++;
    }
}

uint64_t rxdelaysum = 0;
uint64_t rxrttsum = 0;

void
ClientRxDelay (const Time &delay, const Address &from)
{
  if (httplatlog.is_open())
    httplatlog <<  Simulator::Now().GetNanoSeconds() << ", " << delay.GetNanoSeconds() << std::endl;

  rxdelaysum += delay.GetMicroSeconds();
}

void
ClientRxRtt (const Time &delay, const Address &from)
{
  rxrttsum += delay.GetMicroSeconds();
}


void ParseBWP(std::string & p) 
{
  std::istringstream ss(p);
  int c = 0;
  std::string d;
  uint64_t r = 0, pe = 0, dir = 0;

  while (ss) {
    std::string next;
    if (!getline(ss, next, ',')) break;

    if (c == 0) {
      r = std::stoull(next);
    }
    else if (c == 1) {
      d = next;
    }
    else if (c == 2) {
      pe = std::stoull(next);
    }
    else if (c == 3) {
      dir = std::stoull(next);
      bwt.emplace_back(r, d, pe, dir==1);
      c = 0;
      continue;
    }

    c++;
  }
}

void SetHttpVariable(Ptr<ThreeGppHttpVariables> & httpVariables) {
  httpVariables->SetReadingTimeMean (Time(NanoSeconds(10.0)));
  httpVariables->SetParsingTimeMean (Time(NanoSeconds(10.0)));
  httpVariables->SetAttribute("LowMtuSize", UintegerValue(1400));
  httpVariables->SetAttribute("HighMtuSize", UintegerValue(1400));
  httpVariables->SetAttribute("MainObjectSizeMin", UintegerValue(objszmin));
  httpVariables->SetAttribute("MainObjectSizeMax", UintegerValue(objszmax));
  httpVariables->SetAttribute("MainObjectSizeMean", UintegerValue(objszmean));
  httpVariables->SetAttribute("MainObjectSizeStdDev", UintegerValue(objszsd));
  // httpVariables->SetAttribute("EmbeddedObjectSizeMin", UintegerValue(512));
  // httpVariables->SetAttribute("EmbeddedObjectSizeMax", UintegerValue(512000));
  httpVariables->SetAttribute("NumOfEmbeddedObjectsMax", UintegerValue(1));
  httpVariables->SetAttribute("NumOfEmbeddedObjectsScale", UintegerValue(0));
  httpVariables->SetAttribute("RequestSize", UintegerValue(reqsz));
}

int main(int argc, char * argv[]) {

  CommandLine cmd;
  cmd.AddValue("BWP", "Bandwidth pattern", bwt_str);
  cmd.AddValue("QueueLength", "Queue length of router", q_size);
  cmd.AddValue("HostRate", "Link rate between host and TOR", h_rate);
  cmd.AddValue("HostPropDelay", "Host propagation delay", hdelay);
  cmd.AddValue("Static", "If the topology should be static. Will use the first linkrate in specified pattern", nochange);
  cmd.AddValue("SimTime", "Max simulation time", simtime);
  cmd.AddValue("SendTime", "How long does client turn on", sendtime);
  cmd.AddValue("RWND", "Receiver windown size", rwnd);
  cmd.AddValue("NFlows", "Number of flows", nflows);
  cmd.AddValue("Nsd", "standard deviation for flow start time", nsd);
  cmd.AddValue("Rjitter", "BW rate Jitter.", rjitter);
  cmd.AddValue("Tjitter", "BW frequency Jitter.", tjitter);
  cmd.AddValue("ObjSzMax", "HTTP response size max.", objszmax);
  cmd.AddValue("ObjSzMin", "HTTP response size min.", objszmin);
  cmd.AddValue("ObjSzMean", "HTTP response size mean.", objszmean);
  cmd.AddValue("ObjSzSD", "HTTP response size std dev.", objszsd);
  cmd.AddValue("RequestSz", "HTTP request size.", reqsz);
  cmd.AddValue("IndirHopMean", "Indirection mean hops.", hopm);
  cmd.AddValue("IndirHopSD", "SD of indirection hopes.", hops);
  cmd.AddValue("IndirHopBound", "bound of indirection hops.", hopb);
  cmd.Parse(argc, argv);

  ParseBWP(bwt_str);

  std::random_device rd;
  gen = std::default_random_engine(rd());

  if (((int)rjitter) <= 0)
    rjitter_enable = false;
  else
    TopoHelper::rjitnd = std::normal_distribution<double>(0, rjitter);

  if (((int)tjitter) <= 0)
    tjitter_enable = false;
  else
    tjitnd = std::normal_distribution<double>(0, tjitter);

  TopoHelper::hopnd->SetAttribute ("Mean", DoubleValue (hopm));
  TopoHelper::hopnd->SetAttribute ("Variance", DoubleValue (hops));
  TopoHelper::hopnd->SetAttribute ("Bound", DoubleValue (hopb));

  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);
  std::ostringstream oss;

  oss.str("");
  oss << "./httplat" << std::put_time(&tm, "_%m_%d_%Y_%H_%M_%S.csv");
  httplatlog.open(oss.str());

  // oss.str("");
  // oss << "./frlog" << std::put_time(&tm, "_%m_%d_%Y_%H_%M_%S");
  // frlog.open(oss.str());

  // oss.str("");
  // oss << "./lolog" << std::put_time(&tm, "_%m_%d_%Y_%H_%M_%S");
  // lolog.open(oss.str());

  /*
     oss.str("");
     oss << "./cwndlog" << std::put_time(&tm, "_%m_%d_%Y_%H_%M_%S");
     cwndlog.open(oss.str());
     */

  oss.str("");
  oss << "./config" << std::put_time(&tm, "_%m_%d_%Y_%H_%M_%S.json");
  config.open(oss.str());

  config << "{" << std::endl;
  config << "\t\"timestamp\":\t" << std::put_time(&tm, "\"%m_%d_%Y_%H_%M_%S\",") << std::endl;
  config << "\t\"host_rate\":\t"<< h_rate << "," << std::endl;
  config << "\t\"host_propdelay\":\t\"" << hdelay << "\"," << std::endl;
  config << "\t\"sendtime\":\t" << sendtime << "," << std::endl;
  config << "\t\"queue_length\":\t" << q_size << "," << std::endl;
  config << "\t\"rwnd\":\t" << rwnd << "," << std::endl;
  config << "\t\"nflows\":\t" << nflows << "," << std::endl;
  config << "\t\"objszmin\":\t" << objszmin << "," << std::endl;
  config << "\t\"objszmax\":\t" << objszmax << "," << std::endl;
  config << "\t\"objszmean\":\t" << objszmean << "," << std::endl;
  config << "\t\"objszsd\":\t" << objszsd << "," << std::endl;
  config << "\t\"reqsz\":\t" << reqsz << "," << std::endl;
  config << "\t\"rate_jitter\":\t" << (rjitter_enable ? rjitter : 0) << "," << std::endl;
  config << "\t\"dt_jitter\":\t" << (tjitter_enable ? tjitter : 0 )<< "," << std::endl;
  config << "\t\"static\":\t" << nochange << "," << std::endl;

  config << "\t\"bwp\":\t[" << std::endl;  

  for (uint64_t k = 0; k < bwt.size(); k++) {
    auto & item = bwt[k];
    config << "\t\t{\"rate\": " << item.bandwidth << 
      ", \"delay\": \"" << item.delay_s << 
      "\", \"period\": " << item.period << 
      ", \"direct\": " << item.direct << "}" << (k==bwt.size()-1 ? "" : ",") << std::endl;  
  }

  config << "\t]," << std::endl;  

  Time::SetResolution(Time::NS);

  //Packet::EnablePrinting();

  TopoHelper::Init(4);

  PointToPointHelper hp2p;
  PointToPointHelper sp2p;
  hp2p.SetDeviceAttribute("DataRate", DataRateValue(DataRate(h_rate)));
  hp2p.SetDeviceAttribute("Mtu", UintegerValue(1514));
  hp2p.SetChannelAttribute("Delay", StringValue(hdelay));
  // hp2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
  TopoHelper::Connect(hp2p, 0, 2);
  TopoHelper::Connect(hp2p, 1, 3);

  sp2p.SetDeviceAttribute("DataRate", DataRateValue(DataRate(bwt[0].bandwidth)));
  sp2p.SetDeviceAttribute("Mtu", UintegerValue(1514));
  sp2p.SetChannelAttribute("Delay", StringValue(bwt[0].delay_s));
  sp2p.SetChannelType("PointToPointOrderedChannel");
  sp2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));

  TopoHelper::Connect(sp2p, 2, 3);

  InternetStackHelper stack;
  stack.Install(TopoHelper::allNodes);

  TrafficControlHelper tch;
  uint16_t handle = tch.SetRootQueueDisc("ns3::FifoQueueDisc");
  tch.AddInternalQueues(handle, 1, "ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize(BYTES, (q_size*1514))));
  tch.Install(TopoHelper::GetLink(2, 3).Get(0));
  tch.Install(TopoHelper::GetLink(2, 3).Get(1));

  Ipv4AddressHelper address;
  address.SetBase("10.0.0.0", "255.255.255.0");
  TopoHelper::AssignIP(address, 0, 2);
  address.SetBase("10.0.1.0", "255.255.255.0");
  TopoHelper::AssignIP(address, 1, 3);
  address.SetBase("10.1.0.0", "255.255.255.0");
  TopoHelper::AssignIP(address, 2, 3);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();

  // frdata = std::vector<uint64_t>(nflows*2, 0);
  // lodata = std::vector<uint64_t>(nflows*2, 0);
  // syndata = std::vector<uint64_t>(nflows*2, 0);

  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (1448));

  Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue (rwnd));
  Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue (rwnd));

  std::normal_distribution<double> nd(0.0, nsd);
  double startdiff;

  Address sinkAddressr(InetSocketAddress(TopoHelper::GetIf(1, 3).GetAddress(0)));
  std::vector< Ptr<BulkSendApplication> > sendAppsr;

  Address sinkAddressl(InetSocketAddress(TopoHelper::GetIf(0, 2).GetAddress(0)));
  std::vector< Ptr<BulkSendApplication> > sendAppsl;

  // Create HTTP server helper
  ThreeGppHttpServerHelper serverHelper (TopoHelper::GetIf(1, 3).GetAddress(0));

  // Install HTTP server
  ApplicationContainer serverApps = serverHelper.Install (TopoHelper::allNodes.Get(1));
  Ptr<ThreeGppHttpServer> httpServer = serverApps.Get (0)->GetObject<ThreeGppHttpServer> ();

  // Example of connecting to the trace sources
  // httpServer->TraceConnectWithoutContext ("ConnectionEstablished",
  //                                         MakeCallback (&ServerConnectionEstablished));
  // httpServer->TraceConnectWithoutContext ("MainObject", MakeCallback (&MainObjectGenerated));
  // httpServer->TraceConnectWithoutContext ("EmbeddedObject", MakeCallback (&EmbeddedObjectGenerated));
  // httpServer->TraceConnectWithoutContext ("Tx", MakeCallback (&ServerTx));

  // Setup HTTP variables for the server
  PointerValue varPtr;
  httpServer->GetAttribute ("Variables", varPtr);
  Ptr<ThreeGppHttpVariables> httpVariables = varPtr.Get<ThreeGppHttpVariables> ();
  SetHttpVariable(httpVariables);

  serverApps.Start (Seconds (0));

  // Create HTTP client helper
  for (uint64_t j = 0; j < nflows; j++) {

    ThreeGppHttpClientHelper clientHelper (TopoHelper::GetIf(1, 3).GetAddress(0));

    // Install HTTP client
    ApplicationContainer clientApps = clientHelper.Install (TopoHelper::allNodes.Get (0));
    Ptr<ThreeGppHttpClient> httpClient = clientApps.Get (0)->GetObject<ThreeGppHttpClient> ();
    PointerValue varPtrCl;
    httpClient->GetAttribute ("Variables", varPtrCl);
    httpVariables = varPtrCl.Get<ThreeGppHttpVariables> ();
    SetHttpVariable(httpVariables);

    httpClient->TraceConnectWithoutContext ("RxMainObject", MakeCallback (&ClientMainObjectReceived));
    httpClient->TraceConnectWithoutContext ("RxEmbeddedObject", MakeCallback (&ClientEmbeddedObjectReceived));
    httpClient->TraceConnectWithoutContext ("RxDelay", MakeCallback (&ClientRxDelay));
    httpClient->TraceConnectWithoutContext ("RxRtt", MakeCallback (&ClientRxRtt));

    if ((startdiff = nd(gen)) < 1)
      startdiff = -startdiff;

    clientApps.Start (MicroSeconds (1000000.0 + startdiff));
    clientApps.Stop (MicroSeconds ((sendtime + 1) * 1000000.0 + startdiff));
  }

  if (!nochange)
    Simulator::Schedule(MicroSeconds(1000000+bwt[0].period), CycleRate);

  Simulator::Stop(Seconds(simtime));
  Simulator::Run();

  oss.str("");
  oss << "mon" << std::put_time(&tm, "_%m_%d_%Y_%H_%M_%S") << ".xml";
  flowMonitor->SerializeToXmlFile(oss.str(), true, true);

  Simulator::Destroy();

  // config << "\t\"flowdata\":\t[" << std::endl;

  // if (bidir)
  //   nflows *= 2;

  // for (uint64_t k = 0; k < nflows; k++) {
  //   config << "\t\t{" << std::endl; 
  //   config << "\t\t\t\"port\":\t" << (1 + k + sendPortBase) << "," << std::endl; 
  //   config << "\t\t\t\"retransmit\":\t" << frdata[k] << "," << std::endl; 
  //   config << "\t\t\t\"timeout\":\t" << lodata[k] << "," << std::endl; 
  //   config << "\t\t\t\"syn_sent\":\t" << syndata[k] << std::endl; 
  //   config << "\t\t}" << (k == nflows-1 ? "" : ",") << std::endl; 
  // }
  // config << "\t]" << std::endl;
  config << "\t\"main_obj_success\":\t" << mainObjSuccess << "," << std::endl;
  config << "\t\"main_obj_fail\":\t" << mainObjBad << "," << std::endl;
  config << "\t\"embedded_obj_success\":\t" << embObjSuccess << "," << std::endl;
  config << "\t\"embedded_obj_fail\":\t" << embObjBad << "," << std::endl;
  config << "\t\"rx_delay_sum\":\t" << rxdelaysum << "," << std::endl;
  config << "\t\"rx_rtt_sum\":\t" << rxrttsum << std::endl;

  config << "}" << std::endl;  

  // qllog.close();
  // frlog.close();
  // lolog.close();
  // cwndlog.close();

  config.close();

  return 0;

}
