#include "RecoEgamma/PhotonIdentification/interface/TLEMVAEstimatorRun2Fall15.h"
#include <LeptonIsolationHelper/Helper/interface/LeptonIsoHelper.h>

#include "FWCore/ParameterSet/interface/FileInPath.h"

#include "TMVA/MethodBDT.h"

TLEMVAEstimatorRun2Fall15::TLEMVAEstimatorRun2Fall15(const edm::ParameterSet& conf):
  AnyMVAEstimatorRun2Base(conf),
  _MethodName("BDTG method"),
  _useValueMaps(conf.getParameter<bool>("useValueMaps")),
  _full5x5SigmaIEtaIEtaMapLabel(conf.getParameter<edm::InputTag>("full5x5SigmaIEtaIEtaMap")), 
  _full5x5SigmaIEtaIPhiMapLabel(conf.getParameter<edm::InputTag>("full5x5SigmaIEtaIPhiMap")), 
  _full5x5E1x3MapLabel(conf.getParameter<edm::InputTag>("full5x5E1x3Map")), 
  _full5x5E2x2MapLabel(conf.getParameter<edm::InputTag>("full5x5E2x2Map")), 
  _full5x5E2x5MaxMapLabel(conf.getParameter<edm::InputTag>("full5x5E2x5MaxMap")), 
  _full5x5E5x5MapLabel(conf.getParameter<edm::InputTag>("full5x5E5x5Map")), 
  _esEffSigmaRRMapLabel(conf.getParameter<edm::InputTag>("esEffSigmaRRMap")), 
  _phoChargedIsolationLabel(conf.getParameter<edm::InputTag>("phoChargedIsolation")), 
  _phoPhotonIsolationLabel(conf.getParameter<edm::InputTag>("phoPhotonIsolation")), 
  _phoWorstChargedIsolationLabel(conf.getParameter<edm::InputTag>("phoWorstChargedIsolation")), 
  _rhoLabel(conf.getParameter<edm::InputTag>("rho"))
{
 sampleType = 2015;
 setup = sampleType;
// rhoForEleToken = consumes<double>(LeptonIsoHelper::getEleRhoTag(sampleType, setup));


  //
  // Construct the MVA estimators
  //
  _tag = conf.getParameter<std::string>("mvaTag");

  const std::vector <std::string> weightFileNames
    = conf.getParameter<std::vector<std::string> >("weightFileNames");

  if( (int)(weightFileNames.size()) != NUM_CAT )
    throw cms::Exception("MVA config failure: ")
      << "wrong number of weightfiles" << std::endl;

  _gbrForests.clear();
  // The method name is just a key to retrieve this method later, it is not
  // a control parameter for a reader (the full definition of the MVA type and
  // everything else comes from the xml weight files).
   
  // Create a TMVA reader object for each category
  for(int i=0; i < NUM_CAT; i++){

    // Use unique_ptr so that all readers are properly cleaned up
    // when the vector clear() is called in the destructor

    edm::FileInPath weightFile( weightFileNames[i] );
    _gbrForests.push_back( createSingleReader(i, weightFile ) );

  }

}

TLEMVAEstimatorRun2Fall15::
~TLEMVAEstimatorRun2Fall15(){
}

float TLEMVAEstimatorRun2Fall15::
mvaValue(const edm::Ptr<reco::Candidate>& particle, const edm::Event& iEvent) const {  

  const int iCategory = findCategory( particle );
  const std::vector<float> vars = std::move( fillMVAVariables( particle, iEvent ) );  
  
  const float result = _gbrForests.at(iCategory)->GetClassifier(vars.data());

  // DEBUG
  const bool debug = false;
  if( debug ){
    printf("Printout of the photon variable inputs for MVA:\n");
    printf("  varPhi_           %f\n", vars[0]   );
    printf("  varR9_            %f\n", vars[1]   ); 
    printf("  varSieie_         %f\n", vars[2]   );
    printf("  varSieip_         %f\n", vars[3]   ); 
    printf("  varE1x3overE5x5_  %f\n", vars[4]   ); 
    printf("  varE2x2overE5x5_  %f\n", vars[5]   ); 
    printf("  varE2x5overE5x5_  %f\n", vars[6]   ); 
    printf("  varSCEta_         %f\n", vars[7]   ); 
    printf("  varRawE_          %f\n", vars[8]   ); 
    printf("  varSCEtaWidth_    %f\n", vars[9]   ); 
    printf("  varSCPhiWidth_    %f\n", vars[10]  ); 
    printf("  varRho_           %f\n", vars[11]  );
    printf("  varPhoIsoRaw_     %f\n", vars[12]  );
    printf("  varChIsoRaw_      %f\n", vars[13]  ); 
    printf("  varWorstChRaw_    %f\n", vars[14]  );
    if( isEndcapCategory( iCategory ) ) {
      printf("  varESEnOverRawE_  %f\n", vars[15]  ); // for endcap MVA only
      printf("  varESEffSigmaRR_  %f\n", vars[16]  ); // for endcap MVA only
      // The spectators
      printf("  varPt_    %f\n", vars[17]          ); 
      printf("  varEta_  %f\n",  vars[18]          );
    } else {
      // The spectators
     // printf("  varPt_    %f\n", vars[15]          ); 
     // printf("  varEta_  %f\n",  vars[16]          );
    }    
  }
  
  return result;
}

int TLEMVAEstimatorRun2Fall15::findCategory( const edm::Ptr<reco::Candidate>& particle) const {
  
  // Try to cast the particle into a reco particle.
  // This should work for both reco and pat.
  const edm::Ptr<reco::Photon> phoRecoPtr = ( edm::Ptr<reco::Photon> )particle;
  if( phoRecoPtr.isNull() )
    throw cms::Exception("MVA failure: ")
      << " given particle is expected to be reco::Photon or pat::Photon," << std::endl
      << " but appears to be neither" << std::endl;

  float eta = phoRecoPtr->superCluster()->eta();

  //
  // Determine the category
  //
  int  iCategory = UNDEFINED;
  const float ebeeSplit = 1.479; // division between barrel and endcap

  if ( std::abs(eta) < 0.8)  
    iCategory = CAT_EB1;

  if ( std::abs(eta) >= 0.8 && std::abs(eta) < ebeeSplit)  
    iCategory = CAT_EB2;

  if (std::abs(eta) >= ebeeSplit) 
    iCategory = CAT_EE;

  return iCategory;
}

bool TLEMVAEstimatorRun2Fall15::
isEndcapCategory(int category) const {

  // For this specific MVA the function is trivial, but kept for possible
  // future evolution to an MVA with more categories in eta
  bool isEndcap = false;
  if( category == CAT_EE )
    isEndcap = true;

  return isEndcap;
}


std::unique_ptr<const GBRForest> TLEMVAEstimatorRun2Fall15::
createSingleReader(const int iCategory, const edm::FileInPath &weightFile){

  //
  // Create the reader  
  //
  TMVA::Reader tmpTMVAReader( "!Color:Silent:Error" );

  //
  // Configure all variables and spectators. Note: the order and names
  // must match what is found in the xml weights file!
  //

  tmpTMVAReader.AddVariable("ele_oldsigmaietaieta", &_allMVAVars.ele_oldsigmaietaieta);
  tmpTMVAReader.AddVariable("ele_oldsigmaiphiiphi", &_allMVAVars.ele_oldsigmaiphiiphi);
  tmpTMVAReader.AddVariable("ele_oldcircularity"   , &_allMVAVars.ele_oldcircularity);
  tmpTMVAReader.AddVariable("ele_oldr9"        , &_allMVAVars.ele_oldr9);
  //tmpTMVAReader.AddVariable("e1x3Full5x5/e5x5Full5x5", &_allMVAVars.varE1x3overE5x5);
  //tmpTMVAReader.AddVariable("e2x2Full5x5/e5x5Full5x5", &_allMVAVars.varE2x2overE5x5);
  //tmpTMVAReader.AddVariable("e2x5Full5x5/e5x5Full5x5", &_allMVAVars.varE2x5overE5x5);
  //tmpTMVAReader.AddVariable("recoSCEta" , &_allMVAVars.varSCEta);
  //tmpTMVAReader.AddVariable("rawE"      , &_allMVAVars.varRawE);
  tmpTMVAReader.AddVariable("ele_scletawidth", &_allMVAVars.ele_scletawidth);
  tmpTMVAReader.AddVariable("ele_sclphiwidth", &_allMVAVars.ele_sclphiwidth);

  // Endcap only variables
  // Pileup
  //tmpTMVAReader.AddVariable("rho"       , &_allMVAVars.varRho);

  // Isolations
  tmpTMVAReader.AddVariable("ele_he" , &_allMVAVars.ele_he);
  tmpTMVAReader.AddVariable("ele_HZZ_iso"  , &_allMVAVars.ele_HZZ_iso);
  tmpTMVAReader.AddVariable("ele_hasPixelSeed", &_allMVAVars.ele_hasPixelSeed);
  tmpTMVAReader.AddVariable("ele_passElectronVeto", &_allMVAVars.ele_passElectronVeto);
//  tmpTMVAReader.AddVariable("", &_allMVAVars.);

  if( isEndcapCategory(iCategory) ){
    tmpTMVAReader.AddVariable("ele_psEoverEraw" , &_allMVAVars.ele_psEoverEraw);
    //tmpTMVAReader.AddVariable("esRR"      , &_allMVAVars.varESEffSigmaRR);
  }


//  tmpTMVAReader.AddVariable("", &_allMVAVars.);

  // Spectators
  //tmpTMVAReader.AddSpectator("recoPt" , &_allMVAVars.varPt);
  //tmpTMVAReader.AddSpectator("recoEta", &_allMVAVars.varEta);

  //
  // Book the method and set up the weights file
  //
  std::unique_ptr<TMVA::IMethod> temp( tmpTMVAReader.BookMVA(_MethodName , weightFile.fullPath() ) );

  return std::unique_ptr<const GBRForest>( new GBRForest( dynamic_cast<TMVA::MethodBDT*>( tmpTMVAReader.FindMVA(_MethodName) ) ) );

}

// A function that should work on both pat and reco objects
std::vector<float> TLEMVAEstimatorRun2Fall15::fillMVAVariables(const edm::Ptr<reco::Candidate>& particle, const edm::Event& iEvent) const {
  
  // 
  // Declare all value maps corresponding to the above tokens
  //
  edm::Handle<edm::ValueMap<float> > full5x5SigmaIEtaIEtaMap;
  edm::Handle<edm::ValueMap<float> > full5x5SigmaIEtaIPhiMap;
  edm::Handle<edm::ValueMap<float> > full5x5E1x3Map;
  edm::Handle<edm::ValueMap<float> > full5x5E2x2Map;
  edm::Handle<edm::ValueMap<float> > full5x5E2x5MaxMap;
  edm::Handle<edm::ValueMap<float> > full5x5E5x5Map;
  edm::Handle<edm::ValueMap<float> > esEffSigmaRRMap;
  //
  edm::Handle<edm::ValueMap<float> > phoChargedIsolationMap;
  edm::Handle<edm::ValueMap<float> > phoPhotonIsolationMap;
  edm::Handle<edm::ValueMap<float> > phoWorstChargedIsolationMap;

  // Rho will be pulled from the event content
  edm::Handle<double> rho;

  // Get the full5x5 and ES maps
  if( _useValueMaps ) {
    iEvent.getByLabel(_full5x5SigmaIEtaIEtaMapLabel, full5x5SigmaIEtaIEtaMap);
    iEvent.getByLabel(_full5x5SigmaIEtaIPhiMapLabel, full5x5SigmaIEtaIPhiMap);
    iEvent.getByLabel(_full5x5E1x3MapLabel, full5x5E1x3Map);
    iEvent.getByLabel(_full5x5E2x2MapLabel, full5x5E2x2Map);
    iEvent.getByLabel(_full5x5E2x5MaxMapLabel, full5x5E2x5MaxMap);
    iEvent.getByLabel(_full5x5E5x5MapLabel, full5x5E5x5Map);
    iEvent.getByLabel(_esEffSigmaRRMapLabel, esEffSigmaRRMap);
  }

  // Get the isolation maps
  iEvent.getByLabel(_phoChargedIsolationLabel, phoChargedIsolationMap);
  iEvent.getByLabel(_phoPhotonIsolationLabel, phoPhotonIsolationMap);
  iEvent.getByLabel(_phoWorstChargedIsolationLabel, phoWorstChargedIsolationMap);

  // Get rho
  iEvent.getByLabel(_rhoLabel,rho);

  // Make sure everything is retrieved successfully
  if(! ( ( !_useValueMaps || ( full5x5SigmaIEtaIEtaMap.isValid()
                               && full5x5SigmaIEtaIPhiMap.isValid()
                               && full5x5E1x3Map.isValid()
                               && full5x5E2x2Map.isValid()
                               && full5x5E2x5MaxMap.isValid()
                               && full5x5E5x5Map.isValid()
                               && esEffSigmaRRMap.isValid() ) )
         && phoChargedIsolationMap.isValid()
         && phoPhotonIsolationMap.isValid()
         && phoWorstChargedIsolationMap.isValid()
         && rho.isValid() ) )
    throw cms::Exception("MVA failure: ")
      << "Failed to retrieve event content needed for this MVA" 
      << std::endl
      << "Check python MVA configuration file and make sure all needed"
      << std::endl
      << "producers are running upstream" << std::endl;

  // Try to cast the particle into a reco particle.
  // This should work for both reco and pat.
  const edm::Ptr<reco::Photon> phoRecoPtr(particle);
  if( phoRecoPtr.isNull() )
    throw cms::Exception("MVA failure: ")
      << " given particle is expected to be reco::Photon or pat::Photon," << std::endl
      << " but appears to be neither" << std::endl;

  // Both pat and reco particles have exactly the same accessors.
  auto superCluster = phoRecoPtr->superCluster();
  // Full 5x5 cluster shapes. We could take some of this directly from
  // the photon object, but some of these are not available.
  //float e1x3 = std::numeric_limits<float>::max();
  float e1x5 = std::numeric_limits<float>::max();
  //float e2x2 = std::numeric_limits<float>::max();
  //float e2x5 = std::numeric_limits<float>::max();
  float e5x5 = std::numeric_limits<float>::max();
  float full5x5_sigmaIetaIeta = std::numeric_limits<float>::max();
  float full5x5_sigmaIetaIphi = std::numeric_limits<float>::max();
  //float effSigmaRR = std::numeric_limits<float>::max();

  AllVariables allMVAVars;

  if( _useValueMaps ) { //(before 752)
    // in principle, in the photon object as well
    // not in the photon object 
    //e1x3 = (*full5x5E1x3Map   )[ phoRecoPtr ];
    //e2x2 = (*full5x5E2x2Map   )[ phoRecoPtr ];
    //e2x5 = (*full5x5E2x5MaxMap)[ phoRecoPtr ];
    e5x5 = (*full5x5E5x5Map   )[ phoRecoPtr ];
    full5x5_sigmaIetaIeta = (*full5x5SigmaIEtaIEtaMap)[ phoRecoPtr ];
    full5x5_sigmaIetaIphi = (*full5x5SigmaIEtaIPhiMap)[ phoRecoPtr ];
    //effSigmaRR = (*esEffSigmaRRMap)[ phoRecoPtr ];
  } else {
    // from 753
    const auto& full5x5_pss = phoRecoPtr->full5x5_showerShapeVariables();
   // e1x3 = full5x5_pss.e1x3;
    e1x5 = full5x5_pss.e1x5;

    //e2x2 = full5x5_pss.e2x2;
    //e2x5 = full5x5_pss.e2x5Max;
    e5x5 = full5x5_pss.e5x5;
    full5x5_sigmaIetaIeta = full5x5_pss.sigmaIetaIeta;
    full5x5_sigmaIetaIphi = full5x5_pss.sigmaIetaIphi;
    //effSigmaRR = full5x5_pss.effSigmaRR;
  }
  double rhoForEle;
  //edm::Handle<double> rhoHandle;
  //iEvent.getByToken(rhoForEleToken, rhoHandle);
  rhoForEle = *rho; //*rhoHandle;

  const pat::Photon* pat_pho = dynamic_cast<const pat::Photon*>(&*particle);
  float fsr = 0;
  float ele_HZZ_iso = LeptonIsoHelper::combRelIsoPF(sampleType, setup, rhoForEle, *pat_pho, fsr);

  float ele_olde55 = e5x5;
  float ele_olde15 = e1x5; 
  float ele_oldcircularity = ele_olde55 != 0. ? 1. - (ele_olde15 / ele_olde55) : -1.;

  //allMVAVars.varPhi          = phoRecoPtr->phi();
  allMVAVars.ele_oldr9            = phoRecoPtr->r9() ;
  allMVAVars.ele_oldsigmaietaieta = full5x5_sigmaIetaIeta; 
  allMVAVars.ele_oldsigmaiphiiphi = full5x5_sigmaIetaIphi;
  allMVAVars.ele_he               = phoRecoPtr->hadronicOverEm();
  allMVAVars.ele_HZZ_iso          = ele_HZZ_iso;
  allMVAVars.ele_hasPixelSeed     = phoRecoPtr->hasPixelSeed(); 
  allMVAVars.ele_passElectronVeto = pat_pho->passElectronVeto(); 
  allMVAVars.ele_scletawidth      = superCluster->etaWidth(); 
  allMVAVars.ele_sclphiwidth      = superCluster->phiWidth(); 
  allMVAVars.ele_psEoverEraw      = superCluster->preshowerEnergy() / superCluster->rawEnergy();
  allMVAVars.ele_oldcircularity   = ele_oldcircularity;
//  allMVAVars.varRho          = *rho; 
//  allMVAVars.varPhoIsoRaw    = (*phoPhotonIsolationMap)[phoRecoPtr];  
//  allMVAVars.varChIsoRaw     = (*phoChargedIsolationMap)[phoRecoPtr];
//  allMVAVars.varWorstChRaw   = (*phoWorstChargedIsolationMap)[phoRecoPtr];
  // Declare spectator vars




  constrainMVAVariables(allMVAVars);

  std::vector<float> vars;
  if( isEndcapCategory( findCategory( particle ) ) ) {
    vars = std::move( packMVAVariables(allMVAVars.ele_oldsigmaietaieta,
                                       allMVAVars.ele_oldsigmaiphiiphi,
                                       allMVAVars.ele_oldcircularity,
                                       allMVAVars.ele_oldr9,
                                       allMVAVars.ele_scletawidth,
                                       allMVAVars.ele_sclphiwidth,
                                       allMVAVars.ele_he,
                                       allMVAVars.ele_HZZ_iso,
                                       allMVAVars.ele_hasPixelSeed,
                                       allMVAVars.ele_passElectronVeto,
                                       allMVAVars.ele_psEoverEraw)
//                                       allMVAVars.varEta) 
                      ); 
  } else {
    vars = std::move( packMVAVariables(allMVAVars.ele_oldsigmaietaieta,
                                       allMVAVars.ele_oldsigmaiphiiphi,
                                       allMVAVars.ele_oldcircularity,
                                       allMVAVars.ele_oldr9,
                                       allMVAVars.ele_scletawidth,
                                       allMVAVars.ele_sclphiwidth,
                                       allMVAVars.ele_he,
                                       allMVAVars.ele_HZZ_iso,
                                       allMVAVars.ele_hasPixelSeed,
                                       allMVAVars.ele_passElectronVeto)
                      );
  }
  return vars;
}

void TLEMVAEstimatorRun2Fall15::constrainMVAVariables(AllVariables&)const {

  // Check that variables do not have crazy values
  
  // This function is currently empty as this specific MVA was not
  // developed with restricting variables to specific physical ranges.
  return;

}

void TLEMVAEstimatorRun2Fall15::setConsumes(edm::ConsumesCollector&& cc) const {
  if( _useValueMaps ) {
    cc.consumes<edm::ValueMap<float> >(_full5x5SigmaIEtaIEtaMapLabel);
    cc.consumes<edm::ValueMap<float> >(_full5x5SigmaIEtaIPhiMapLabel); 
    cc.consumes<edm::ValueMap<float> >(_full5x5E1x3MapLabel); 
    cc.consumes<edm::ValueMap<float> >(_full5x5E2x2MapLabel);
    cc.consumes<edm::ValueMap<float> >(_full5x5E2x5MaxMapLabel);
    cc.consumes<edm::ValueMap<float> >(_full5x5E5x5MapLabel);
    cc.consumes<edm::ValueMap<float> >(_esEffSigmaRRMapLabel);
  }
  cc.consumes<edm::ValueMap<float> >(_phoChargedIsolationLabel);
  cc.consumes<edm::ValueMap<float> >(_phoPhotonIsolationLabel);
  cc.consumes<edm::ValueMap<float> >( _phoWorstChargedIsolationLabel);
  cc.consumes<double>(_rhoLabel);
}

