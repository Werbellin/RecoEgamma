#ifndef RecoEgamma_PhotonIdentification_TLEMVAEstimatorRun2Fall15_H
#define RecoEgamma_PhotonIdentification_TLEMVAEstimatorRun2Fall15_H

#include "FWCore/Framework/interface/ConsumesCollector.h"

#include "DataFormats/Common/interface/ValueMap.h"

#include "RecoEgamma/EgammaTools/interface/AnyMVAEstimatorRun2Base.h"

#include "DataFormats/EgammaCandidates/interface/Photon.h"

#include "CondFormats/EgammaObjects/interface/GBRForest.h"

#include <vector>
#include <string>
#include <TROOT.h>
#include "TMVA/Factory.h"
#include "TMVA/Tools.h"
#include "TMVA/Reader.h"

class TLEMVAEstimatorRun2Fall15 : public AnyMVAEstimatorRun2Base{
  
 public:

  // Define here the number and the meaning of the categories
  // for this specific MVA
//  const int nCategories = 2;
  enum mvaCategories {
    UNDEFINED = -1,
    CAT_EB1  = 0,
    CAT_EB2  = 1,
    CAT_EE   = 2,
    NUM_CAT  = 3
  };

  // Define the struct that contains all necessary for MVA variables
  struct AllVariables {
   float ele_oldsigmaietaieta;
   float ele_oldsigmaiphiiphi;
   float ele_oldcircularity;
   float ele_oldr9;
   float ele_scletawidth;
   float ele_sclphiwidth;
   float ele_he;
   float ele_HZZ_iso;
   float ele_hasPixelSeed;
   float ele_passElectronVeto;
   float ele_psEoverEraw; 
 };
  
  // Constructor and destructor
  TLEMVAEstimatorRun2Fall15(const edm::ParameterSet& conf);
  ~TLEMVAEstimatorRun2Fall15();

  // Calculation of the MVA value
  float mvaValue( const edm::Ptr<reco::Candidate>& particle, const edm::Event&) const;
 
  // Utility functions
  std::unique_ptr<const GBRForest> createSingleReader(const int iCategory, const edm::FileInPath &weightFile);
  
  virtual int getNCategories() const { return NUM_CAT; }
  bool isEndcapCategory( int category ) const;
  virtual const std::string& getName() const override final { return _name; }
  virtual const std::string& getTag() const override final { return _tag; }
  
  // Functions that should work on both pat and reco electrons
  // (use the fact that pat::Electron inherits from reco::GsfElectron)
  std::vector<float> fillMVAVariables(const edm::Ptr<reco::Candidate>& particle, const edm::Event& iEvent) const override;
  int findCategory( const edm::Ptr<reco::Candidate>& particle ) const override;
  // The function below ensures that the variables passed to MVA are 
  // within reasonable bounds
  void constrainMVAVariables(AllVariables&) const;

  // Call this function once after the constructor to declare
  // the needed event content pieces to the framework
  void setConsumes(edm::ConsumesCollector&&) const override;
  // Call this function once per event to retrieve all needed
  // event content pices
  //void getEventContent(const edm::Event& iEvent) override;

  
 private:

  // MVA name. This is a unique name for this MVA implementation.
  // It will be used as part of ValueMap names.
  // For simplicity, keep it set to the class name.
  const std::string _name = "TLEMVAEstimatorRun2Fall15";

  // MVA tag. This is an additional string variable to distinguish
  // instances of the estimator of this class configured with different
  // weight files.
  std::string _tag;

  // Data members
  std::vector< std::unique_ptr<const GBRForest> > _gbrForests;

  // All variables needed by this MVA
  const std::string _MethodName;
  AllVariables _allMVAVars;
  
  // This MVA implementation relies on several ValueMap objects
  // produced upstream. 

  //
  // Declare all tokens that will be needed to retrieve misc
  // data from the event content required by this MVA
  //
  const bool _useValueMaps;
  const edm::InputTag _full5x5SigmaIEtaIEtaMapLabel; 
  const edm::InputTag _full5x5SigmaIEtaIPhiMapLabel; 
  const edm::InputTag _full5x5E1x3MapLabel; 
  const edm::InputTag _full5x5E2x2MapLabel; 
  const edm::InputTag _full5x5E2x5MaxMapLabel; 
  const edm::InputTag _full5x5E5x5MapLabel; 
  const edm::InputTag _esEffSigmaRRMapLabel; 
  //
  const edm::InputTag _phoChargedIsolationLabel; 
  const edm::InputTag _phoPhotonIsolationLabel; 
  const edm::InputTag _phoWorstChargedIsolationLabel; 
  const edm::InputTag _rhoLabel;

        edm::EDGetTokenT<double> rhoForEleToken;
      int sampleType;
      int setup;

};

DEFINE_EDM_PLUGIN(AnyMVAEstimatorRun2Factory,
		  TLEMVAEstimatorRun2Fall15,
		  "TLEMVAEstimatorRun2Fall15");

#endif
