#include <sstream>
#include <vector>
#include <TStyle.h>
#include <TAxis.h>
#include <TLatex.h>
#include <TPaveText.h>
#include <TFile.h>
#include <TTree.h>
#include <TString.h>
#include <TChain.h>
#include <TCanvas.h>
#include <TNtupleD.h>
#include <TH1D.h>
#include <TLorentzVector.h>
#include <TLegend.h>
#include <TSystem.h>
#include <RooWorkspace.h>
#include <RooRealVar.h>
#include <RooConstVar.h>
#include <RooDataSet.h>
#include <RooGaussian.h>
#include <RooChebychev.h>
#include <RooBernstein.h>
#include <RooExponential.h>
#include <RooAddPdf.h>
#include <RooPlot.h>
#include "UserCode/B_production_x_sec_13_TeV/interface/myloop.h"
#include "UserCode/B_production_x_sec_13_TeV/interface/plotDressing.h"
#include "TMath.h"
using namespace RooFit;

#define MASS_MIN_2 5.0
#define MASS_MAX_2 5.6


//-----------------------------------------------------------------
// Definition of channel #
// channel = 1: B+ -> J/psi K+
// channel = 2: B0 -> J/psi K*
// channel = 3: B0 -> J/psi Ks
// channel = 4: Bs -> J/psi phi
// channel = 5: Jpsi + pipi
// channel = 6: Lambda_b -> Jpsi + Lambda

void plot_pt_dist(RooWorkspace& w, int channel, TString directory);
void plot_mass_dist(RooWorkspace& w, int channel, TString directory);

void read_data(RooWorkspace& w, TString filename,int channel);
void read_data_cut(RooWorkspace& w, RooDataSet* data);
void set_up_workspace_variables(RooWorkspace& w, int channel);
TString channel_to_ntuple_name(int channel);

std::vector<TH1D*> sideband_sub(RooWorkspace& w, double left, double right, int mc);

void data_selection(TString fin1,TString data_selection_output_file,int channel,int mc);

//input example: data_selection --channel 1 --input /some/place/ --sub 1 --showdist 1 --mc 0 --select 1
int main(int argc, char** argv)
{
  int channel = 0;
  std::string input_file = "/lstore/cms/brunogal/input_for_B_production_x_sec_13_TeV/myloop_data.root";
  bool side_sub = 0, show_dist = 0, select = 1;
  int mc = 0;

  for(int i=1 ; i<argc ; ++i)
    {
      std::string argument = argv[i];
      std::stringstream convert;

      if(argument == "--channel")
        {
          convert << argv[++i];
          convert >> channel;
        }

      if(argument == "--select")
	{
	  convert << argv[++i];
	  convert >> select;
	}

      if(argument == "--input")
        {
          convert << argv[++i];
          convert >> input_file;
        }

      if(argument == "--sub")
	{
	  convert << argv[++i];
	  convert >> side_sub;
	}

      if(argument == "--showdist")
	{
	  convert << argv[++i];
	  convert >> show_dist;
	}

      if(argument == "--mc")
	{
	  convert << argv[++i];
	  convert >> mc;
	}
    }

  if(channel==0)
    {
      std::cout << "No channel was provided as input. Please use --channel. Example: data_selection --channel 1" << std::endl;
      return 0;
    }

  TString input_file_mc="/lstore/cms/brunogal/input_for_B_production_x_sec_13_TeV/myloop_new_"+channel_to_ntuple_name(channel)+"_bmuonfilter_with_cuts.root";
//  TString input_file_mc_2="myloop_new_"+channel_to_ntuple_name(channel)+"_bmuonfilter_with_cuts.root";


  TString data_selection_output_file="";
  TString data_selection_output_file_mc="";
  data_selection_output_file= "selected_data_" + channel_to_ntuple_name(channel) + ".root";
  data_selection_output_file_mc= "selected_data_" + channel_to_ntuple_name(channel) + "_mc.root";
  
  if(select)  data_selection(input_file,data_selection_output_file,channel, 0);
  if(mc==1) data_selection(input_file_mc, data_selection_output_file_mc, channel, mc);

  if(show_dist)
    { 
      RooWorkspace* ws = new RooWorkspace("ws","Bmass");
      TString pt_dist_directory="";
      TString mass_dist_directory="";
 
      gSystem->Exec("mkdir -p full_dataset_mass_pt_dist/");

      //set up mass and pt variables inside ws  
      set_up_workspace_variables(*ws,channel);
      
      //read data from the selected data file, and import it as a dataset into the workspace.
      read_data(*ws, data_selection_output_file,channel);
      
      RooAbsData* data = ws->data("data");
      
      pt_dist_directory = "full_dataset_mass_pt_dist/" + channel_to_ntuple_name(channel) + "_pt";
      plot_pt_dist(*ws,channel,pt_dist_directory);
      
      mass_dist_directory = "full_dataset_mass_pt_dist/" + channel_to_ntuple_name(channel) + "_mass";
      plot_mass_dist(*ws,channel,mass_dist_directory);
    }

  if(side_sub)
    {
      RooWorkspace* ws = new RooWorkspace("ws","Bmass");
      //set up mass and pt variables inside ws  
      set_up_workspace_variables(*ws,channel);

      //read data from the selected data file, and import it as a dataset into the workspace.
      read_data(*ws, data_selection_output_file,channel);
      
      std::vector<TH1D*> histos_data;
      std::vector<TH1D*> histos_mc;


      switch(channel)
	{
	case 2:
	  histos_data = sideband_sub(*ws, 5.1, 5.4, 0);
	  break;
	case 4:
	  histos_data = sideband_sub(*ws, 5.25, 5.45, 0);
	  break;
	default:
	  std::cout << "WARNING! UNDEFINED LIMITS FOR PEAK REGION" << std::endl;
	}

      RooWorkspace* ws_mc = new RooWorkspace("ws_mc","Bmass");
      //set up mass and pt variables inside ws  
      set_up_workspace_variables(*ws_mc,channel);

      //read data from the selected data file, and import it as a dataset into the workspace.
      read_data(*ws_mc, data_selection_output_file_mc,channel);

      switch(channel)
	{
	case 2:
	  histos_mc = sideband_sub(*ws_mc, 5.1, 5.4, 1);
	  break;
	case 4:
	  histos_mc = sideband_sub(*ws_mc, 5.22, 5.5, 1);
	  break;
	default:
	  std::cout << "WARNING! UNDEFINED LIMITS FOR PEAK REGION" << std::endl;
	}

      std::string names[] = {"pt", "mu1pt", "mu2pt", "mu1eta", "mu2eta", "y", "vtxprob", "lxy", "errlxy", "lerrxy"};    

      for(int i=0; i<10; i++)
	{
	  TCanvas c;
	  
	  if(channel==2)
	    {
	      histos_data[i]->Scale(1/histos_data[i]->Integral());
	      histos_data[i]->Draw();
	      histos_mc[i]->Scale(1/histos_mc[i]->Integral());
	      histos_mc[i]->Draw("same");
	    }
	  else if(channel==4)
	    {
	      histos_mc[i]->Scale(1/histos_mc[i]->Integral());
	      histos_data[i]->Scale(1/histos_data[i]->Integral());
	      histos_data[i]->Draw();
	      histos_mc[i]->Draw("same");
	    }
	  
	  /*	  if(i==6)
	    {
	      histos_data[i]->GetYaxis()->SetRangeUser(0.1, 1000);
	      histos_mc[i]->GetYaxis()->SetRangeUser(0.1, 1000); 
	      }*/
	  
	  if(i<3) c.SetLogy();
	  
	  TLegend* leg;
	  
	  if(i>2 && i<7) leg = new TLegend (0.15, 0.8, 0.4, 0.9);
	  else leg = new TLegend (0.6, 0.5, 0.85, 0.75);
	  
	  leg->AddEntry(histos_data[i]->GetName(), "Sideband Subtraction", "l");
	  leg->AddEntry(histos_mc[i]->GetName(), "Monte Carlo", "l");
	  leg->SetTextSize(0.03);
	  //  std::cout<<"NOME DO DATA: "<< histos_data[i]->GetName()<< std::endl;
	  // std::cout<<"NOME DO MC: "<< histos_mc[i]->GetName()<< std::endl;
	  leg->Draw("same");

    TLatex* tex = new TLatex(0.68,0.8,"2.71 fb^{-1} (13 TeV)");
    tex->SetNDC(kTRUE);
    tex->SetLineWidth(2);
    tex->SetTextSize(0.04);
    tex->Draw();
    tex = new TLatex(0.68,0.85,"CMS Preliminary");
    tex->SetNDC(kTRUE);
    tex->SetTextFont(42);
    tex->SetTextSize(0.04);
    tex->SetLineWidth(2);
    tex->Draw();


	  
	  c.SaveAs((names[i]+"_mc_validation.png").c_str());
	}
      
    }
}

void plot_pt_dist(RooWorkspace& w, int channel, TString directory)
{
  //full dataset pt distribution
  RooRealVar pt = *(w.var("pt"));
  RooAbsData* data = w.data("data");
  
  TCanvas c2;
  TH1D* pt_dist = (TH1D*)data->createHistogram("pt_dist",pt);
  pt_dist->Draw();
  c2.SetLogy();
  c2.SaveAs(directory + ".root");
  c2.SaveAs(directory + ".png");
}

void plot_mass_dist(RooWorkspace& w, int channel, TString directory)
{
  //full dataset mass distribution
  RooRealVar mass = *(w.var("mass"));
  RooAbsData* data = w.data("data");

  TCanvas c2;
  TH1D* mass_dist = (TH1D*)data->createHistogram("mass_dist",mass);
  mass_dist->Draw();
  c2.SaveAs(directory + ".root");
  c2.SaveAs(directory + ".png");
}

void read_data(RooWorkspace& w, TString filename,int channel)
{

  TFile* f = new TFile(filename);
  TNtupleD* _nt = (TNtupleD*)f->Get(channel_to_ntuple_name(channel));
 
  RooArgSet arg_list(*(w.var("mass")) , *(w.var("pt")) , *(w.var("y")) , *(w.var("mu1pt")) , *(w.var("mu2pt")) , *(w.var("mu1eta")) , *(w.var("mu2eta")) , *(w.var("lxy")) , *(w.var("errxy")) );

  arg_list.add(*(w.var("vtxprob")));
  arg_list.add(*(w.var("lerrxy")));

  RooDataSet* data = new RooDataSet("data","data",_nt,arg_list);
  
  w.import(*data);
}

void read_data_cut(RooWorkspace& w, RooDataSet* data)
{
  w.import(*data);
}

void set_up_workspace_variables(RooWorkspace& w, int channel)
{
  double mass_min, mass_max;
  double pt_min, pt_max;
  double y_min, y_max;
  double mu1pt_min, mu1pt_max;
  double mu2pt_min, mu2pt_max;
  double mu1eta_min, mu1eta_max;
  double mu2eta_min, mu2eta_max;
  double lxy_min, lxy_max;
  double errxy_min, errxy_max;
  double vtxprob_min, vtxprob_max;
  double lerrxy_min, lerrxy_max;

  pt_min=0;
  pt_max=300;
  
  y_min=-3;
  y_max=3;

  mu1pt_min=0;
  mu1pt_max=90;

  mu2pt_min=0;
  mu2pt_max=120;

  mu1eta_min=-3;
  mu1eta_max=3;

  mu2eta_min=-3;
  mu2eta_max=3;

  lxy_min=0;
  lxy_max=2;

  errxy_min=0;
  errxy_max=0.05;

  vtxprob_min=0.2;
  vtxprob_max=1;

  lerrxy_min=0;
  lerrxy_max=100;

  switch (channel) {
  default:
  case 1:
    mass_min = 5.0; mass_max = 6.0;
    break;
  case 2:
    mass_min = MASS_MIN_2; mass_max = MASS_MAX_2;
    break;
  case 3:
    mass_min = 5.0; mass_max = 6.0;
    break;
  case 4:
    mass_min = 5.0; mass_max = 6.0;
    break;
  case 5:
    mass_min = 3.6; mass_max = 4.0;
    break;
  case 6:
    mass_min = 5.3; mass_max = 6.3;
    break;
  }

  RooRealVar mass("mass","mass",mass_min,mass_max);
  RooRealVar pt("pt","pt",pt_min,pt_max);
  RooRealVar y("y","y",y_min,y_max);
  RooRealVar mu1pt("mu1pt","mu1pt",mu1pt_min,mu1pt_max);
  RooRealVar mu2pt("mu2pt","mu2pt",mu2pt_min,mu2pt_max);
  RooRealVar mu1eta("mu1eta","mu1eta",mu1eta_min,mu1eta_max);
  RooRealVar mu2eta("mu2eta","mu2eta",mu2eta_min,mu2eta_max);
  RooRealVar lxy("lxy","lxy",lxy_min,lxy_max);
  RooRealVar errxy("errxy","errxy",errxy_min,errxy_max);
  RooRealVar vtxprob("vtxprob","vtxprob",vtxprob_min,vtxprob_max);
  RooRealVar lerrxy("lerrxy","lerrxy",lerrxy_min,lerrxy_max);

  w.import(mass);
  w.import(pt);
  w.import(y);
  w.import(mu1pt);
  w.import(mu2pt);
  w.import(mu1eta);
  w.import(mu2eta);
  w.import(lxy);
  w.import(errxy);
  w.import(vtxprob);
  w.import(lerrxy);
}

void data_selection(TString fin1, TString data_selection_output_file,int channel,int mc)
{
  TFile *fout = new TFile(data_selection_output_file,"recreate");

  TNtupleD *_nt1 = new TNtupleD("ntkp","ntkp","mass:pt:eta:y:mu1pt:mu2pt:mu1eta:mu2eta:lxy:errxy:vtxprob:lerrxy");
  TNtupleD *_nt2 = new TNtupleD("ntkstar","ntkstar","mass:pt:eta:y:mu1pt:mu2pt:mu1eta:mu2eta:lxy:errxy:vtxprob:lerrxy");
  TNtupleD *_nt3 = new TNtupleD("ntks","ntks","mass:pt:eta:y:mu1pt:mu2pt:mu1eta:mu2eta:lxy:errxy:vtxprob:lerrxy");
  TNtupleD *_nt4 = new TNtupleD("ntphi","ntphi","mass:pt:eta:y:mu1pt:mu2pt:mu1eta:mu2eta:lxy:errxy:vtxprob:lerrxy");
  TNtupleD *_nt5 = new TNtupleD("ntmix","ntmix","mass:pt:eta:y:mu1pt:mu2pt:mu1eta:mu2eta:lxy:errxy:vtxprob:lerrxy");
  TNtupleD *_nt6 = new TNtupleD("ntlambda","ntlambda","mass:pt:eta:y:mu1pt:mu2pt:mu1eta:mu2eta:lxy:errxy:vtxprob:lerrxy");

  /*
    switch (channel) {
    case 1:
    default:
    _nt1 = new TNtupleD("ntkp","ntkp","mass:pt:eta");
    break;
    case 2:
    _nt2 = new TNtupleD("ntkstar","ntkstar","mass:pt:eta");
    break;
    case 3:
    _nt3 = new TNtupleD("ntks","ntks","mass:pt:eta");
    break;
    case 4:
    _nt4 = new TNtupleD("ntphi","ntphi","mass:pt:eta");
    break;
    case 5:
    _nt5 = new TNtupleD("ntmix","ntmix","mass:pt:eta");
    break;
    case 6:
    _nt6 = new TNtupleD("ntlambda","ntlambda","mass:pt:eta");
    break;
    }
  */

  ReducedBranches br;
  TChain* tin;
  
  int n_br_queued = 0;
  ReducedBranches br_queue[32];
  TLorentzVector v4_tk1, v4_tk2;
  
  std::cout << "selecting data from channel " << channel << std::endl;

  tin = new TChain(channel_to_ntuple_name(channel));

  tin->Add(fin1);
  br.setbranchadd(tin);

  for (int evt=0;evt<tin->GetEntries();evt++) {
    tin->GetEntry(evt);
        
    if (channel==1) { // cuts for B+ -> J/psi K+
      if (br.hltbook[HLT_DoubleMu4_JpsiTrk_Displaced_v2]!=1) continue;
      if (br.vtxprob<=0.1) continue;//original cut 0.1
      if (br.tk1pt<=1.6) continue;//original cut 1.6
      if (br.lxy/br.errxy<=3.0) continue;//original cut 3.0
      if (br.cosalpha2d<=0.99) continue;//original cut 0.99
            
      _nt1->Fill(br.mass,br.pt,br.eta,br.y,br.mu1pt,br.mu2pt,br.mu1eta,br.mu2eta,br.lxy,br.errxy,br.vtxprob, br.lxy/br.errxy);
	    
    }else
      if (channel==2) { // cuts for B0 -> J/psi K*
	if (mc!=1 && br.hltbook[HLT_DoubleMu4_JpsiTrk_Displaced_v2]!=1) continue;
	if (br.vtxprob<=0.2) continue;//original cut 0.1
	if (br.lxy/br.errxy<=4.5) continue;//original cut 3.0
	if (br.cosalpha2d<=0.996) continue;//original cut 0.99
	if (fabs(br.tktkmass-KSTAR_MASS)>=0.05) continue;//original cut 0.05
            
	v4_tk1.SetPtEtaPhiM(br.tk1pt,br.tk1eta,br.tk1phi,KAON_MASS);
	v4_tk2.SetPtEtaPhiM(br.tk2pt,br.tk2eta,br.tk2phi,KAON_MASS);
	if (fabs((v4_tk1+v4_tk2).Mag()-PHI_MASS)<=0.01) continue;//original cut 0.01
            
	if (n_br_queued==0) {
	  memcpy(&br_queue[n_br_queued],&br,sizeof(ReducedBranches));
	  n_br_queued++;
	}else
	  if (br.run == br_queue[n_br_queued-1].run && br.event == br_queue[n_br_queued-1].event) { // same event
	    memcpy(&br_queue[n_br_queued],&br,sizeof(ReducedBranches));
	    n_br_queued++;
	    if (n_br_queued>=32) printf("Warning: maximum queued branches reached.\n");
	  }            
	if (br.run != br_queue[n_br_queued-1].run || br.event != br_queue[n_br_queued-1].event || evt==tin->GetEntries()-1) {
	  for (int i=0; i<n_br_queued; i++) {
                    
	    bool isBestKstarMass = true;
	    for (int j=0; j<n_br_queued; j++) {
	      if (j==i) continue;
	      if (br_queue[i].mu1idx==br_queue[j].mu1idx &&
		  br_queue[i].mu2idx==br_queue[j].mu2idx &&
		  br_queue[i].tk1idx==br_queue[j].tk1idx &&
		  br_queue[i].tk2idx==br_queue[j].tk2idx) {
                        
		if (fabs(br_queue[j].tktkmass-KSTAR_MASS)<fabs(br_queue[i].tktkmass-KSTAR_MASS)) {
		  isBestKstarMass = false;
		  continue;
		}
	      }
	    }
                                 
	    if (isBestKstarMass){
	      _nt2->Fill(br_queue[i].mass,br_queue[i].pt,br_queue[i].eta,br_queue[i].y,
			 br_queue[i].mu1pt,br_queue[i].mu2pt,br_queue[i].mu1eta,br_queue[i].mu2eta,
			 br_queue[i].lxy,br_queue[i].errxy,br_queue[i].vtxprob, br_queue[i].lxy/br_queue[i].errxy);

	    }

	  }                
	  n_br_queued = 0;
	  memcpy(&br_queue[n_br_queued],&br,sizeof(ReducedBranches));
	  n_br_queued++;
	}
      }else
	if (channel==3) { // cuts for B0 -> J/psi Ks
	  if (br.hltbook[HLT_DoubleMu4_JpsiTrk_Displaced_v2]!=1) continue;
	  if (br.vtxprob<=0.1) continue;//original cut 0.1
	  if (br.lxy/br.errxy<=3.0) continue;//original cut 3.0
	  if (br.tktkblxy/br.tktkberrxy<=3.0) continue;//original cut 3.0
	  if (br.cosalpha2d<=0.99) continue;//original cut 0.99
	  if (fabs(br.tktkmass-KSHORT_MASS)>=0.015) continue;//original cut 0.015
                
	  _nt3->Fill(br.mass,br.pt,br.eta,br.y,br.mu1pt,br.mu2pt,br.mu1eta,br.mu2eta,br.lxy,br.errxy,br.vtxprob,br.lxy/br.errxy);

	}else
	  if (channel==4) { // cuts for Bs -> J/psi phi
	    if (mc!=1 && br.hltbook[HLT_DoubleMu4_JpsiTrk_Displaced_v2]!=1) continue;
	    if (br.vtxprob<=0.2) continue;//original cut 0.1
	    if (br.lxy/br.errxy<=4.5) continue;//original cut 3.0
	    if (br.cosalpha2d<=0.996) continue;//original cut 0.99
	    if (fabs(br.tktkmass-PHI_MASS)>=0.010) continue;//original cut 0.010
            
	    v4_tk1.SetPtEtaPhiM(br.tk1pt,br.tk1eta,br.tk1phi,KAON_MASS);
	    v4_tk2.SetPtEtaPhiM(br.tk2pt,br.tk2eta,br.tk2phi,PION_MASS);
	    if (fabs((v4_tk1+v4_tk2).Mag()-KSTAR_MASS)<=0.05) continue;//original cut 0.05
	    v4_tk1.SetPtEtaPhiM(br.tk1pt,br.tk1eta,br.tk1phi,PION_MASS);
	    v4_tk2.SetPtEtaPhiM(br.tk2pt,br.tk2eta,br.tk2phi,KAON_MASS);
	    if (fabs((v4_tk1+v4_tk2).Mag()-KSTAR_MASS)<=0.05) continue;
	    
	    _nt4->Fill(br.mass,br.pt,br.eta,br.y,br.mu1pt,br.mu2pt,br.mu1eta,br.mu2eta,br.lxy,br.errxy,br.vtxprob, br.lxy/br.errxy);

	  }else
	    if (channel==5) { // cuts for psi(2S)/X(3872) -> J/psi pipi
	      if (br.vtxprob<=0.2) continue;//original cut 0.2
	      if (fabs(br.tk1eta)>=1.6) continue;//original cut 1.6
	      if (fabs(br.tk2eta)>=1.6) continue;//original cut 1.6
            
	      _nt5->Fill(br.mass,br.pt,br.eta,br.y,br.mu1pt,br.mu2pt,br.mu1eta,br.mu2eta,br.lxy,br.errxy,br.vtxprob, br.lxy/br.errxy);

	    }else
	      if (channel==6) {//cuts for lambda
		if (br.hltbook[HLT_DoubleMu4_JpsiTrk_Displaced_v2]!=1) continue;
		if (br.vtxprob<=0.1) continue;//original cut 0.1
		if (br.lxy/br.errxy<=3.0) continue;//original cut 3.0
		if (br.tktkblxy/br.tktkberrxy<=3.0) continue;//original cut 3.0
		if (br.cosalpha2d<=0.99) continue;//original cut 0.99
		if (fabs(br.tktkmass-LAMBDA_MASS)>=0.015) continue;//original cut 0.015
            
		v4_tk1.SetPtEtaPhiM(br.tk1pt,br.tk1eta,br.tk1phi,PION_MASS);
		v4_tk2.SetPtEtaPhiM(br.tk2pt,br.tk2eta,br.tk2phi,PION_MASS);
		if (fabs((v4_tk1+v4_tk2).Mag()-KSHORT_MASS)<=0.015) continue;//original cut 0.015
            
		_nt6->Fill(br.mass,br.pt,br.eta,br.y,br.mu1pt,br.mu2pt,br.mu1eta,br.mu2eta,br.lxy,br.errxy,br.vtxprob, br.lxy/br.errxy);

	      }
  }//end of the for for the events
  fout->Write();
  fout->Close();
}

TString channel_to_ntuple_name(int channel)
{
  //returns a TString with the ntuple name corresponding to the channel. It can be used to find the data on each channel saved in a file. or to write the name of a directory

  TString ntuple_name = "";

  switch(channel){
  default:
  case 1:
    ntuple_name="ntkp";
    break;
  case 2:
    ntuple_name="ntkstar";
    break;
  case 3:
    ntuple_name="ntks";
    break;
  case 4:
    ntuple_name="ntphi";
    break;
  case 5:
    ntuple_name="ntmix";
    break;
  case 6:
    ntuple_name="ntlambda";
    break;
  }
  return ntuple_name;
}

std::vector<TH1D*> sideband_sub(RooWorkspace& w, double left, double right, int mc)
{
  //Create appropriate variables and data sets (the pt isn't imported from the RooWorkspace because its range will change)                      
  RooRealVar pt = *(w.var("pt"));                                                                                                            
  RooRealVar y = *(w.var("y"));                                                                                                            
  RooRealVar mu1pt = *(w.var("mu1pt"));
  RooRealVar mu2pt = *(w.var("mu2pt"));
  RooRealVar mu1eta = *(w.var("mu1eta"));
  RooRealVar mu2eta = *(w.var("mu2eta"));
  RooRealVar lxy = *(w.var("lxy"));
  RooRealVar errlxy = *(w.var("errxy"));
  RooRealVar vtxprob = *(w.var("vtxprob"));
  RooRealVar lerrxy = *(w.var("lerrxy"));  
  //RooRealVar pt("pt","pt",0.,150.);
  RooRealVar mass = *(w.var("mass"));
  RooDataSet* data =(RooDataSet*) w.data("data");
  RooDataSet* reduceddata_side;  RooDataSet* reduceddata_aux;
  RooDataSet* reduceddata_central;

  //Make selection for the different bands using mass as the selection variable                                                                  

  reduceddata_side = (RooDataSet*) data->reduce(Form("mass<%lf", left));
  reduceddata_aux = (RooDataSet*) data->reduce(Form("mass>%lf",right));

  reduceddata_side->append(*reduceddata_aux);

  reduceddata_central = (RooDataSet*) data->reduce(Form("mass>%lf",left));
  reduceddata_central = (RooDataSet*) reduceddata_central->reduce(Form("mass<%lf",right));
  
  /*RooRealVar mean("mean", "mean", 0., 5.);                                                                                                    
    RooRealVar sigma("sigma", "sigma", -1000., 1000.);*/

  RooRealVar lambda("lambda", "lambda",-5., -20., 0.);
  
  //Bernstein try
/*  RooRealVar m_par1("m_par1","m_par2",1.,0,+10.);
  RooRealVar m_par2("m_par2","m_par3",1.,0,+10.);
  RooRealVar m_par3("m_par3","m_par3",1.,0,+10.);
  RooRealVar m_par4("m_par4","m_par4",1.,0,+10.);
  RooRealVar m_par5("m_par5","m_par5",1.,0,+10.);
  
  RooBernstein fit_side("fit_side","fit_side",mass,RooArgList(RooConst(1.),m_par1,m_par2,m_par3,m_par4, m_par5));*/

  RooExponential fit_side("fit_side", "fit_side_exp", mass, lambda);

  mass.setRange("all", mass.getMin(),mass.getMax());
  mass.setRange("right",right,mass.getMax());
  mass.setRange("left",mass.getMin(),left);
  mass.setRange("peak",left,right);
  
  std::cout<<"mass minimum: "<<mass.getMin()<<std::endl;
  std::cout<<"mass maximum: "<<mass.getMax()<<std::endl;
  

  fit_side.fitTo(*reduceddata_side,Range("left,right"));
  //  RooRealVar* nll = (RooRealVar*) fit_side.createNLL(*reduceddata_side, Range("left,right"));

  RooPlot* massframe = mass.frame(Title("Exponential Fit - Sideband Subtraction"));
//  reduceddata_side->plotOn(massframe);
  data->plotOn(massframe);
  fit_side.plotOn(massframe, Range("all"));
  massframe->GetYaxis()->SetTitleOffset(1.2);
//  massframe->SetNameTitle("sideband_fit", "Exponential Fit - Sideband Subtraction");
  TCanvas d;
  massframe->Draw(); 
  TLatex* tex11 = new TLatex(0.68,0.8,"2.71 fb^{-1} (13 TeV)");
  tex11->SetNDC(kTRUE);
  tex11->SetLineWidth(2);
  tex11->SetTextSize(0.04);
  tex11->Draw();
  tex11 = new TLatex(0.68,0.85,"CMS Preliminary");
  tex11->SetNDC(kTRUE);
  tex11->SetTextFont(42);
  tex11->SetTextSize(0.04);
  tex11->SetLineWidth(2);
  tex11->Draw();

  double lambda_str = lambda.getVal();
  double lambda_err = lambda.getError();
  double chis = massframe->chiSquare();

  TLatex* tex12 = new TLatex(0.15, 0.85, Form("#lambda_{exp} = %.3lf #pm %.3lf",lambda_str,lambda_err));
  tex12->SetNDC(kTRUE);
  tex12->SetTextFont(42);
  tex12->SetTextSize(0.04);
  tex12->Draw();   
  TLatex* tex13 = new TLatex(0.15, 0.8, Form("#chi/DOF = %.3lf",chis));
  tex13->SetNDC(kTRUE);
  tex13->SetTextFont(42);
  tex13->SetTextSize(0.04);
  //tex13->Draw();   

  
  if(mc==0)  d.SaveAs("fit_side.png");

  std::cout << std::endl << "chisquare: " << massframe->chiSquare() << std::endl;
  //  std::cout << "LogLikelihood: " << nll->getVal() << std::endl;

  //Integrating the background distribution                            

  RooAbsReal* int_fit_side_left = fit_side.createIntegral(mass, "left");
  RooAbsReal* int_fit_side_right = fit_side.createIntegral(mass, "right");
  RooAbsReal* int_fit_peak = fit_side.createIntegral(mass, "peak");

  std::cout<< std::endl << "Integral left band: " << int_fit_side_left->getVal() << std::endl;
  std::cout<< std::endl << "Integral right band: " << int_fit_side_right->getVal() << std::endl;

  double factor = (int_fit_peak->getVal())/(int_fit_side_left->getVal()+int_fit_side_right->getVal());

  std::cout << std::endl << "Factor: " << factor << std::endl;

  //Build and draw signal and background distributionsx                

  std::vector<TH1D*> histos;


  TH1D* pt_dist_side = (TH1D*) reduceddata_side->createHistogram("pt_dist_side",pt);
  pt_dist_side->SetMarkerColor(kBlue);
  pt_dist_side->SetLineColor(kBlue);
  pt_dist_side->SetNameTitle("pt_dist_side", "Signal and Background Distributions - p_{T} (B) ");

  TH1D* hist_pt_dist_peak = (TH1D*) reduceddata_central->createHistogram("pt_dist_peak", pt);
  TH1D* pt_dist_peak = new TH1D(*hist_pt_dist_peak);
  if(mc==1){
    pt_dist_peak->SetMarkerColor(kBlack);
    pt_dist_peak->SetLineColor(kBlack);
    pt_dist_peak->SetName("pt_dist_peak_mc");
  }
  else{
    pt_dist_peak->SetMarkerColor(kRed);
    pt_dist_peak->SetLineColor(kRed);
    pt_dist_peak->SetNameTitle("pt_dist_peak", "Signal and Background Distributions - p_{T} (B)");
  } 
  
  /*if(mc==0)*/  histos.push_back(pt_dist_peak);

  TH1D* pt_dist_total = (TH1D*) data->createHistogram("pt_dist_total",pt);
  pt_dist_total->SetMarkerColor(kBlack);
  pt_dist_total->SetLineColor(kBlack);
  pt_dist_total->SetNameTitle("pt_dist_total", "Signal and Background Distributions - p_{T} (B)");

  pt_dist_peak->Add(pt_dist_side, -factor);
  pt_dist_side->Add(pt_dist_side, factor);

  //  if(mc==1)  histos.push_back(pt_dist_total);
  pt_dist_peak->SetStats(0);
  pt_dist_side->SetStats(0);
  pt_dist_total->SetStats(0);
  TCanvas c;

  pt_dist_total->Draw();
  pt_dist_side->Draw("same");
  pt_dist_peak->Draw("same");
  pt_dist_peak->SetXTitle("p_{T} [GeV]");
  pt_dist_side->SetXTitle("p_{T} [GeV]");
  pt_dist_total->SetXTitle("p_{T} [GeV]");
  
  TLatex* tex = new TLatex(0.68,0.8,"2.71 fb^{-1} (13 TeV)");
  tex->SetNDC(kTRUE);
  tex->SetLineWidth(2);
  tex->SetTextSize(0.04);
  tex->Draw();
  tex = new TLatex(0.68,0.85,"CMS Preliminary");
  tex->SetNDC(kTRUE);
  tex->SetTextFont(42);
  tex->SetTextSize(0.04);
  tex->SetLineWidth(2);
  tex->Draw();

  TLegend *leg = new TLegend (0.7, 0.5, 0.85, 0.65);
  leg->AddEntry("pt_dist_total", "Total", "l");
  leg->AddEntry("pt_dist_peak", "Signal", "l");
  leg->AddEntry("pt_dist_side", "Background", "l");
  leg->Draw("same");

  c.SetLogy();
  if(mc==0)  c.SaveAs("pt_sideband_sub.png");


  TH1D* mu1pt_dist_side = (TH1D*) reduceddata_side->createHistogram("mu1pt_dist_side",mu1pt);
  mu1pt_dist_side->SetMarkerColor(kBlue);
  mu1pt_dist_side->SetLineColor(kBlue);
  mu1pt_dist_side->SetNameTitle("mu1pt_dist_side", "Signal and Background Distributions - p_{T} (#mu_{1}) ");

  TH1D* hist_mu1pt_dist_peak = (TH1D*) reduceddata_central->createHistogram("mu1pt_dist_peak", mu1pt);
  TH1D* mu1pt_dist_peak = new TH1D(*hist_mu1pt_dist_peak);
  if(mc==1){
    mu1pt_dist_peak->SetMarkerColor(kBlack);
    mu1pt_dist_peak->SetLineColor(kBlack);
    mu1pt_dist_peak->SetName("mu1pt_dist_peak_mc");

  }
  else{
    mu1pt_dist_peak->SetMarkerColor(kRed);
    mu1pt_dist_peak->SetLineColor(kRed);
    mu1pt_dist_peak->SetNameTitle("mu1pt_dist_peak", "Signal and Background Distributions - p_{T} (#mu_{1})");
  }

  /*if(mc==0)*/  histos.push_back(mu1pt_dist_peak);

  TH1D* mu1pt_dist_total = (TH1D*) data->createHistogram("mu1pt_dist_total",mu1pt);
  mu1pt_dist_total->SetMarkerColor(kBlack);
  mu1pt_dist_total->SetLineColor(kBlack);
  mu1pt_dist_total->SetNameTitle("mu1pt_dist_total", "Signal and Background Distributions - p_{T} (#mu_{1})");
  

  mu1pt_dist_peak->Add(mu1pt_dist_side, -factor);
  mu1pt_dist_side->Add(mu1pt_dist_side, factor);

  //  if(mc==1)  histos.push_back(mu1pt_dist_total);

  mu1pt_dist_peak->SetStats(0);
  mu1pt_dist_side->SetStats(0);
  mu1pt_dist_total->SetStats(0);


  TCanvas c1;

  mu1pt_dist_total->Draw();
  mu1pt_dist_side->Draw("same");
  mu1pt_dist_peak->Draw("same");
  mu1pt_dist_peak->SetXTitle("p_{T} [GeV]");
  mu1pt_dist_side->SetXTitle("p_{T} [GeV]");
  mu1pt_dist_total->SetXTitle("p_{T} [GeV]");

  TLegend *leg1 = new TLegend (0.7, 0.5, 0.85, 0.65);
  leg1->AddEntry("mu1pt_dist_total", "Total", "l");
  leg1->AddEntry("mu1pt_dist_peak", "Signal", "l");
  leg1->AddEntry("mu1pt_dist_side", "Background", "l");
  leg1->Draw("same");

  TLatex* tex1 = new TLatex(0.68,0.8,"2.71 fb^{-1} (13 TeV)");
  tex1->SetNDC(kTRUE);
  tex1->SetLineWidth(2);
  tex1->SetTextSize(0.04);
  tex1->Draw();
  tex1 = new TLatex(0.68,0.85,"CMS Preliminary");
  tex1->SetNDC(kTRUE);
  tex1->SetTextFont(42);
  tex1->SetTextSize(0.04);
  tex1->SetLineWidth(2);
  tex1->Draw();



  c1.SetLogy();
  if(mc==0) c1.SaveAs("mu1pt_sideband_sub.png");

  TH1D* mu2pt_dist_side = (TH1D*) reduceddata_side->createHistogram("mu2pt_dist_side",mu2pt);
  mu2pt_dist_side->SetMarkerColor(kBlue);
  mu2pt_dist_side->SetLineColor(kBlue);
  mu2pt_dist_side->SetNameTitle("mu2pt_dist_side", "Signal and Background Distributions - p_{T} (#mu_{2}) ");

  TH1D* hist_mu2pt_dist_peak = (TH1D*) reduceddata_central->createHistogram("mu2pt_dist_peak", mu2pt);
  TH1D* mu2pt_dist_peak = new TH1D(*hist_mu2pt_dist_peak);
  if(mc==1){
    mu2pt_dist_peak->SetMarkerColor(kBlack);
    mu2pt_dist_peak->SetLineColor(kBlack);
    mu2pt_dist_peak->SetName("mu2pt_dist_peak_mc");
  }

  else{
    mu2pt_dist_peak->SetMarkerColor(kRed);
    mu2pt_dist_peak->SetLineColor(kRed);
    mu2pt_dist_peak->SetNameTitle("mu2pt_dist_peak", "Signal and Background Distributions - p_{T} (#mu_{2})");
  }

  /*if(mc==0) */ histos.push_back(mu2pt_dist_peak);

  TH1D* mu2pt_dist_total = (TH1D*) data->createHistogram("mu2pt_dist_total",mu2pt);
  mu2pt_dist_total->SetMarkerColor(kBlack);
  mu2pt_dist_total->SetLineColor(kBlack);
  mu2pt_dist_total->SetNameTitle("mu2pt_dist_total", "Signal and Background Distributions - p_{T} (#mu_{2})");

  mu2pt_dist_peak->Add(mu2pt_dist_side, -factor);
  mu2pt_dist_side->Add(mu2pt_dist_side, factor);

  //  if(mc==1)  histos.push_back(mu2pt_dist_total);
  mu2pt_dist_peak->SetStats(0);
  mu2pt_dist_side->SetStats(0);
  mu2pt_dist_total->SetStats(0);
  
  
  TCanvas c2;

  mu2pt_dist_total->Draw();
  mu2pt_dist_side->Draw("same");
  mu2pt_dist_peak->Draw("same");
  mu2pt_dist_peak->SetXTitle("p_{T} [GeV]");
  mu2pt_dist_side->SetXTitle("p_{T} [GeV]");
  mu2pt_dist_total->SetXTitle("p_{T} [GeV]");

  TLegend *leg2 = new TLegend (0.7, 0.5, 0.85, 0.65);
  leg2->AddEntry("mu2pt_dist_total", "Total", "l");
  leg2->AddEntry("mu2pt_dist_peak", "Signal", "l");
  leg2->AddEntry("mu2pt_dist_side", "Background", "l");
  leg2->Draw("same");

  TLatex* tex2 = new TLatex(0.68,0.8,"2.71 fb^{-1} (13 TeV)");
  tex2->SetNDC(kTRUE);
  tex2->SetLineWidth(2);
  tex2->SetTextSize(0.04);
  tex2->Draw();
  tex2 = new TLatex(0.68,0.85,"CMS Preliminary");
  tex2->SetNDC(kTRUE);
  tex2->SetTextFont(42);
  tex2->SetTextSize(0.04);
  tex2->SetLineWidth(2);
  tex2->Draw();


  c2.SetLogy();
  if(mc==0) c2.SaveAs("mu2pt_sideband_sub.png");

  TH1D* mu1eta_dist_side = (TH1D*) reduceddata_side->createHistogram("mu1eta_dist_side",mu1eta);
  mu1eta_dist_side->SetMarkerColor(kBlue);
  mu1eta_dist_side->SetLineColor(kBlue);
  mu1eta_dist_side->SetNameTitle("mu1eta_dist_side", "Signal and Background Distributions - #eta (#mu_{1}) ");

  TH1D* hist_mu1eta_dist_peak = (TH1D*) reduceddata_central->createHistogram("mu1eta_dist_peak", mu1eta);
  TH1D* mu1eta_dist_peak = new TH1D(*hist_mu1eta_dist_peak);
  if(mc==1){
    mu1eta_dist_peak->SetMarkerColor(kBlack);
    mu1eta_dist_peak->SetLineColor(kBlack);
    mu1eta_dist_peak->SetName("mu1eta_dist_peak_mc");

  }
  else{
    mu1eta_dist_peak->SetMarkerColor(kRed);
    mu1eta_dist_peak->SetLineColor(kRed);
    mu1eta_dist_peak->SetNameTitle("mu1eta_dist_peak", "Signal and Background Distributions - #eta (#mu_{1})");
  }

  /*if(mc==0) */ histos.push_back(mu1eta_dist_peak);

  TH1D* mu1eta_dist_total = (TH1D*) data->createHistogram("mu1eta_dist_total",mu1eta);
  mu1eta_dist_total->SetMarkerColor(kBlack);
  mu1eta_dist_total->SetLineColor(kBlack);
  mu1eta_dist_total->SetNameTitle("mu1eta_dist_total", "Signal and Background Distributions - #eta (#mu_{1})");

  mu1eta_dist_peak->Add(mu1eta_dist_side, -factor);
  mu1eta_dist_side->Add(mu1eta_dist_side, factor);

  //if(mc==1)  histos.push_back(mu1eta_dist_total);
  mu1eta_dist_peak->SetStats(0);
  mu1eta_dist_side->SetStats(0);
  mu1eta_dist_total->SetStats(0);

  TCanvas c3;

  mu1eta_dist_total->Draw();
  mu1eta_dist_side->Draw("same");
  mu1eta_dist_peak->Draw("same");
  mu1eta_dist_peak->SetXTitle("#eta");
  mu1eta_dist_side->SetXTitle("#eta");
  mu1eta_dist_total->SetXTitle("#eta");

  TLegend *leg3 = new TLegend (0.15, 0.8, 0.3, 0.9);
  leg3->AddEntry("mu1eta_dist_total", "Total", "l");
  leg3->AddEntry("mu1eta_dist_peak", "Signal", "l");
  leg3->AddEntry("mu1eta_dist_side", "Background", "l");
  leg3->Draw("same");


  TLatex* tex4 = new TLatex(0.68,0.8,"2.71 fb^{-1} (13 TeV)");
  tex4->SetNDC(kTRUE);
  tex4->SetLineWidth(2);
  tex4->SetTextSize(0.04);
  tex4->Draw();
  tex4 = new TLatex(0.68,0.85,"CMS Preliminary");
  tex4->SetNDC(kTRUE);
  tex4->SetTextFont(42);
  tex4->SetTextSize(0.04);
  tex4->SetLineWidth(2);
  tex4->Draw();

  // c3.SetLogy();
  if(mc==0) c3.SaveAs("mu1eta_sideband_sub.png");

  TH1D* mu2eta_dist_side = (TH1D*) reduceddata_side->createHistogram("mu2eta_dist_side",mu2eta);
  mu2eta_dist_side->SetMarkerColor(kBlue);
  mu2eta_dist_side->SetLineColor(kBlue);
  mu2eta_dist_side->SetNameTitle("mu2eta_dist_side", "Signal and Background Distributions - #eta (#mu_{2}) ");

  TH1D* hist_mu2eta_dist_peak = (TH1D*) reduceddata_central->createHistogram("mu2eta_dist_peak", mu2eta);
  TH1D* mu2eta_dist_peak = new TH1D(*hist_mu2eta_dist_peak);
  if(mc==1){
    mu2eta_dist_peak->SetMarkerColor(kBlack);
    mu2eta_dist_peak->SetLineColor(kBlack);
    mu2eta_dist_peak->SetName("mu2eta_dist_peak_mc");

  }  
  else{
    mu2eta_dist_peak->SetMarkerColor(kRed);
    mu2eta_dist_peak->SetLineColor(kRed);
    mu2eta_dist_peak->SetNameTitle("mu2eta_dist_peak", "Signal and Background Distributions - #eta (#mu_{2})");
  }

  /*  if(mc==0)*/  histos.push_back(mu2eta_dist_peak);

  TH1D* mu2eta_dist_total = (TH1D*) data->createHistogram("mu1eta_dist_total",mu2eta);
  mu2eta_dist_total->SetMarkerColor(kBlack);
  mu2eta_dist_total->SetLineColor(kBlack);
  mu2eta_dist_total->SetNameTitle("mu2eta_dist_total", "Signal and Background Distributions - #eta (#mu_{2})");

  mu2eta_dist_peak->Add(mu2eta_dist_side, -factor);
  mu2eta_dist_side->Add(mu2eta_dist_side, factor);

  // if(mc==1)  histos.push_back(mu2eta_dist_total);
  mu2eta_dist_peak->SetStats(0);
  mu2eta_dist_side->SetStats(0);
  mu2eta_dist_total->SetStats(0);

  TCanvas c4;

  mu2eta_dist_total->Draw();
  mu2eta_dist_side->Draw("same");
  mu2eta_dist_peak->Draw("same");
  mu2eta_dist_peak->SetXTitle("#eta");
  mu2eta_dist_side->SetXTitle("#eta");
  mu2eta_dist_total->SetXTitle("#eta");

  TLegend *leg4 = new TLegend (0.15, 0.8, 0.3, 0.9);
  leg4->AddEntry("mu2eta_dist_total", "Total", "l");
  leg4->AddEntry("mu2eta_dist_peak", "Signal", "l");
  leg4->AddEntry("mu2eta_dist_side", "Background", "l");
  leg4->Draw("same");


  TLatex* tex5 = new TLatex(0.68,0.8,"2.71 fb^{-1} (13 TeV)");
  tex5->SetNDC(kTRUE);
  tex5->SetLineWidth(2);
  tex5->SetTextSize(0.04);
  tex5->Draw();
  tex5 = new TLatex(0.68,0.85,"CMS Preliminary");
  tex5->SetNDC(kTRUE);
  tex5->SetTextFont(42);
  tex5->SetTextSize(0.04);
  tex5->SetLineWidth(2);
  tex5->Draw();

  //c4.SetLogy();
  if(mc==0) c4.SaveAs("mu2eta_sideband_sub.png");

  TH1D* y_dist_side = (TH1D*) reduceddata_side->createHistogram("y_dist_side",y);
  y_dist_side->SetMarkerColor(kBlue);
  y_dist_side->SetLineColor(kBlue);
  y_dist_side->SetNameTitle("y_dist_side", "Signal and Background Distributions - y (B) ");

  TH1D* hist_y_dist_peak = (TH1D*) reduceddata_central->createHistogram("y_dist_peak", y);
  TH1D* y_dist_peak = new TH1D(*hist_y_dist_peak);
  if(mc==1){
    y_dist_peak->SetMarkerColor(kBlack);
    y_dist_peak->SetLineColor(kBlack);
    y_dist_peak->SetName("y_dist_peak_mc");

  }
  else{
    y_dist_peak->SetMarkerColor(kRed);
    y_dist_peak->SetLineColor(kRed);
    y_dist_peak->SetNameTitle("y_dist_peak", "Signal and Background Distributions - y (B)");
  }

  /*if(mc==0) */ histos.push_back(y_dist_peak);


  TH1D* y_dist_total = (TH1D*) data->createHistogram("y_dist_total",y);
  y_dist_total->SetMarkerColor(kBlack);
  y_dist_total->SetLineColor(kBlack);
  y_dist_total->SetNameTitle("y_dist_total", "Signal and Background Distributions - y (B)");

  y_dist_peak->Add(y_dist_side, -factor);
  y_dist_side->Add(y_dist_side, factor);

  //  if(mc==1)  histos.push_back(y_dist_total);
  y_dist_peak->SetStats(0);
  y_dist_side->SetStats(0);
  y_dist_total->SetStats(0);

  TCanvas c5;

  y_dist_total->Draw();
  y_dist_side->Draw("same");
  y_dist_peak->Draw("same");
  y_dist_peak->SetXTitle("y");
  y_dist_side->SetXTitle("y");
  y_dist_total->SetXTitle("y");

  TLegend *leg5 = new TLegend (0.15, 0.8, 0.3, 0.9);
  leg5->AddEntry("y_dist_total", "Total", "l");
  leg5->AddEntry("y_dist_peak", "Signal", "l");
  leg5->AddEntry("y_dist_side", "Background", "l");
  leg5->Draw("same");

  TLatex* tex6 = new TLatex(0.68,0.8,"2.71 fb^{-1} (13 TeV)");
  tex6->SetNDC(kTRUE);
  tex6->SetLineWidth(2);
  tex6->SetTextSize(0.04);
  tex6->Draw();
  tex6 = new TLatex(0.68,0.85,"CMS Preliminary");
  tex6->SetNDC(kTRUE);
  tex6->SetTextFont(42);
  tex6->SetTextSize(0.04);
  tex6->SetLineWidth(2);
  tex6->Draw();


  //c5.SetLogy();
  if(mc==0) c5.SaveAs("y_sideband_sub.png");

  TH1D* vtxprob_dist_side = (TH1D*) reduceddata_side->createHistogram("vtxprob_dist_side",vtxprob);
  vtxprob_dist_side->SetMarkerColor(kBlue);
  vtxprob_dist_side->SetLineColor(kBlue);
  vtxprob_dist_side->SetNameTitle("vtxprob_dist_side", "Signal and Background Distributions -  #chi^{2} prob ");

  TH1D* hist_vtxprob_dist_peak = (TH1D*) reduceddata_central->createHistogram("vtxprob_dist_peak", vtxprob);
  TH1D* vtxprob_dist_peak = new TH1D(*hist_vtxprob_dist_peak);
  if(mc==1){
    vtxprob_dist_peak->SetMarkerColor(kBlack);
    vtxprob_dist_peak->SetLineColor(kBlack);
    vtxprob_dist_peak->SetName("vtxprob_dist_peak_mc");
    //  vtxprob_dist_peak->GetYaxis()->SetRangeUser(0.1, 1000);
  }
  else{
    vtxprob_dist_peak->SetMarkerColor(kRed);
    vtxprob_dist_peak->SetLineColor(kRed);
    vtxprob_dist_peak->SetNameTitle("vtxprob_dist_peak", "Signal and Background Distributions - #chi^{2} prob");
    //vtxprob_dist_peak->GetYaxis()->SetRangeUser(0.1, 1000);
  }
  
  /*if(mc==0) */ histos.push_back(vtxprob_dist_peak);


  TH1D* vtxprob_dist_total = (TH1D*) data->createHistogram("vtxprob_dist_total",vtxprob);
  vtxprob_dist_total->SetMarkerColor(kBlack);
  vtxprob_dist_total->SetLineColor(kBlack);
  vtxprob_dist_total->SetNameTitle("vtxprob_dist_total", "Signal and Background Distributions - #chi^{2} prob");

  vtxprob_dist_peak->Add(vtxprob_dist_side, -factor);
  vtxprob_dist_side->Add(vtxprob_dist_side, factor);

  //  if(mc==1)  histos.push_back(vtxprob_dist_total);
  vtxprob_dist_peak->SetStats(0);
  vtxprob_dist_side->SetStats(0);
  vtxprob_dist_total->SetStats(0);

  TCanvas c6;

  vtxprob_dist_total->Draw();
  vtxprob_dist_side->Draw("same");
  vtxprob_dist_peak->Draw("same");
  vtxprob_dist_total->GetYaxis()->SetRangeUser(0,vtxprob_dist_total->GetMaximum()+200);
  vtxprob_dist_peak->SetXTitle("#chi^{2} prob");
  vtxprob_dist_side->SetXTitle("#chi^{2} prob");
  vtxprob_dist_total->SetXTitle("#chi^{2} prob");

  TLegend *leg6 = new TLegend (0.15, 0.8, 0.3, 0.9);
  leg6->AddEntry("vtxprob_dist_total", "Total", "l");
  leg6->AddEntry("vtxprob_dist_peak", "Signal", "l");
  leg6->AddEntry("vtxprob_dist_side", "Background", "l");
  leg6->Draw("same");

  TLatex* tex7 = new TLatex(0.68,0.8,"2.71 fb^{-1} (13 TeV)");
  tex7->SetNDC(kTRUE);
  tex7->SetLineWidth(2);
  tex7->SetTextSize(0.04);
  tex7->Draw();
  tex7 = new TLatex(0.68,0.85,"CMS Preliminary");
  tex7->SetNDC(kTRUE);
  tex7->SetTextFont(42);
  tex7->SetTextSize(0.04);
  tex7->SetLineWidth(2);
  tex7->Draw();


  //c6.SetLogy();
  if(mc==0) c6.SaveAs("vtxprob_sideband_sub.png");
   
  TH1D* lxy_dist_side = (TH1D*) reduceddata_side->createHistogram("lxy_dist_side",lxy);
  lxy_dist_side->SetMarkerColor(kBlue);
  lxy_dist_side->SetLineColor(kBlue);
  lxy_dist_side->SetNameTitle("lxy_dist_side", "Signal and Background Distributions -  L_{xy} ");

  TH1D* hist_lxy_dist_peak = (TH1D*) reduceddata_central->createHistogram("lxy_dist_peak", lxy);
  TH1D* lxy_dist_peak = new TH1D(*hist_lxy_dist_peak);
  if(mc==1){
    lxy_dist_peak->SetMarkerColor(kBlack);
    lxy_dist_peak->SetLineColor(kBlack);
    lxy_dist_peak->SetName("lxy_dist_peak_mc");
  }
  else{
    lxy_dist_peak->SetMarkerColor(kRed);
    lxy_dist_peak->SetLineColor(kRed);
    lxy_dist_peak->SetNameTitle("lxy_dist_peak", "Signal and Background Distributions - L_{xy} ");
  }

  /*  if(mc==0)*/  histos.push_back(lxy_dist_peak);

  TH1D* lxy_dist_total = (TH1D*) data->createHistogram("lxy_dist_total",lxy);
  lxy_dist_total->SetMarkerColor(kBlack);
  lxy_dist_total->SetLineColor(kBlack);
  lxy_dist_total->SetNameTitle("lxy_dist_total", "Signal and Background Distributions - L_{xy} ");

  lxy_dist_peak->Add(lxy_dist_side, -factor);
  lxy_dist_side->Add(lxy_dist_side, factor);

  //  if(mc==1)  histos.push_back(lxy_dist_total);
  lxy_dist_peak->SetStats(0);
  lxy_dist_side->SetStats(0);
  lxy_dist_total->SetStats(0);

  TCanvas c7;

  lxy_dist_total->Draw();
  lxy_dist_side->Draw("same");
  lxy_dist_peak->Draw("same");
  lxy_dist_peak->SetXTitle("L_{xy} [cm]");
  lxy_dist_side->SetXTitle("L_{xy} [cm]");
  lxy_dist_total->SetXTitle("L_{xy} [cm]");

  TLegend *leg7 = new TLegend (0.7, 0.5, 0.85, 0.65);
  leg7->AddEntry("lxy_dist_total", "Total", "l");
  leg7->AddEntry("lxy_dist_peak", "Signal", "l");
  leg7->AddEntry("lxy_dist_side", "Background", "l");
  leg7->Draw("same");

  TLatex* tex8 = new TLatex(0.68,0.8,"2.71 fb^{-1} (13 TeV)");
  tex8->SetNDC(kTRUE);
  tex8->SetLineWidth(2);
  tex8->SetTextSize(0.04);
  tex8->Draw();
  tex8 = new TLatex(0.68,0.85,"CMS Preliminary");
  tex8->SetNDC(kTRUE);
  tex8->SetTextFont(42);
  tex8->SetTextSize(0.04);
  tex8->SetLineWidth(2);
  tex8->Draw();

  c7.SetLogy();
  if(mc==0) c7.SaveAs("lxy_sideband_sub.png");

  TH1D* errlxy_dist_side = (TH1D*) reduceddata_side->createHistogram("errlxy_dist_side",errlxy);
  errlxy_dist_side->SetMarkerColor(kBlue);
  errlxy_dist_side->SetLineColor(kBlue);
  errlxy_dist_side->SetNameTitle("errlxy_dist_side", "Signal and Background Distributions - #sigma L_{xy} ");

  TH1D* hist_errlxy_dist_peak = (TH1D*) reduceddata_central->createHistogram("errlxy_dist_peak", errlxy);
  TH1D* errlxy_dist_peak = new TH1D(*hist_errlxy_dist_peak);
  if(mc==1){
    errlxy_dist_peak->SetMarkerColor(kBlack);
    errlxy_dist_peak->SetLineColor(kBlack);
    errlxy_dist_peak->SetName("errlxy_dist_peak_mc");
  }
  else{
    errlxy_dist_peak->SetMarkerColor(kRed);
    errlxy_dist_peak->SetLineColor(kRed);
    errlxy_dist_peak->SetNameTitle("errlxy_dist_peak", "Signal and Background Distributions - #sigma L_{xy} ");
  }

  /*  if(mc==0) */ histos.push_back(errlxy_dist_peak);

  
  TH1D* errlxy_dist_total = (TH1D*) data->createHistogram("errlxy_dist_total",errlxy);
  errlxy_dist_total->SetMarkerColor(kBlack);
  errlxy_dist_total->SetLineColor(kBlack);
  errlxy_dist_total->SetNameTitle("errlxy_dist_total", "Signal and Background Distributions - #sigma L_{xy} ");

  errlxy_dist_peak->Add(errlxy_dist_side, -factor);
  errlxy_dist_side->Add(errlxy_dist_side, factor);

  //  if(mc==1)  histos.push_back(errlxy_dist_total);
  errlxy_dist_peak->SetStats(0);
  errlxy_dist_side->SetStats(0);
  errlxy_dist_total->SetStats(0);

  TCanvas c8;

  errlxy_dist_total->Draw();
  errlxy_dist_side->Draw("same");
  errlxy_dist_peak->Draw("same");
  //errlxy_dist_total->GetYaxis()->SetRangeUser(0.5,lerrxy_dist_total->GetMaximum()+2000);
  errlxy_dist_peak->SetXTitle("#sigma L_{xy} [cm]");
  errlxy_dist_side->SetXTitle("#sigma L_{xy} [cm]");
  errlxy_dist_total->SetXTitle("#sigma L_{xy} [cm]");

  TLegend *leg8 = new TLegend (0.7, 0.5, 0.85, 0.65);
  leg8->AddEntry("errlxy_dist_total", "Total", "l");
  leg8->AddEntry("errlxy_dist_peak", "Signal", "l");
  leg8->AddEntry("errlxy_dist_side", "Background", "l");
  leg8->Draw("same");

  TLatex* tex9 = new TLatex(0.68,0.8,"2.71 fb^{-1} (13 TeV)");
  tex9->SetNDC(kTRUE);
  tex9->SetLineWidth(2);
  tex9->SetTextSize(0.04);
  tex9->Draw();
  tex9 = new TLatex(0.68,0.85,"CMS Preliminary");
  tex9->SetNDC(kTRUE);
  tex9->SetTextFont(42);
  tex9->SetTextSize(0.04);
  tex9->SetLineWidth(2);
  tex9->Draw();

  c8.SetLogy();
  if(mc==0) c8.SaveAs("errlxy_sideband_sub.png");


  TH1D* lerrxy_dist_side = (TH1D*) reduceddata_side->createHistogram("lerrxy_dist_side",lerrxy);
  lerrxy_dist_side->SetMarkerColor(kBlue);
  lerrxy_dist_side->SetLineColor(kBlue);
  lerrxy_dist_side->SetNameTitle("lerrxy_dist_side", "Signal and Background Distributions - L_{xy}/#sigma L_{xy} ");

  TH1D* hist_lerrxy_dist_peak = (TH1D*) reduceddata_central->createHistogram("lerrxy_dist_peak", lerrxy);
  TH1D* lerrxy_dist_peak = new TH1D(*hist_lerrxy_dist_peak);
  if(mc==1){
    lerrxy_dist_peak->SetMarkerColor(kBlack);
    lerrxy_dist_peak->SetLineColor(kBlack);
    lerrxy_dist_peak->SetName("lerrxy_dist_peak_mc");
  }
  else{
    lerrxy_dist_peak->SetMarkerColor(kRed);
    lerrxy_dist_peak->SetLineColor(kRed);
    lerrxy_dist_peak->SetNameTitle("lerrxy_dist_peak", "Signal and Background Distributions - L_{xy}/#sigma L_{xy} ");
  }

  /*  if(mc==0)*/  histos.push_back(lerrxy_dist_peak);

  TH1D* lerrxy_dist_total = (TH1D*) data->createHistogram("lerrxy_dist_total",lerrxy);
  lerrxy_dist_total->SetMarkerColor(kBlack);
  lerrxy_dist_total->SetLineColor(kBlack);
  lerrxy_dist_total->SetNameTitle("lerrxy_dist_total", "Signal and Background Distributions - L_{xy}/#sigma L_{xy} ");

  lerrxy_dist_peak->Add(lerrxy_dist_side, -factor);
  lerrxy_dist_side->Add(lerrxy_dist_side, factor);

  //  if(mc==1)  histos.push_back(lerrxy_dist_total);
  lerrxy_dist_peak->SetStats(0);
  lerrxy_dist_side->SetStats(0);
  lerrxy_dist_total->SetStats(0);

  TCanvas c9;

  lerrxy_dist_total->Draw();
  lerrxy_dist_side->Draw("same");
  lerrxy_dist_peak->Draw("same");
  lerrxy_dist_total->GetYaxis()->SetRangeUser(100,lerrxy_dist_total->GetMaximum()+2000);
  lerrxy_dist_peak->SetXTitle("L_{xy}/#sigma L_{xy} ");
  lerrxy_dist_side->SetXTitle("L_{xy}/#sigma L_{xy} ");
  lerrxy_dist_total->SetXTitle("L_{xy}/#sigma L_{xy} ");

  TLegend *leg9 = new TLegend (0.7, 0.5, 0.85, 0.65);
  leg9->AddEntry("lerrxy_dist_total", "Total", "l");
  leg9->AddEntry("lerrxy_dist_peak", "Signal", "l");
  leg9->AddEntry("lerrxy_dist_side", "Background", "l");
  leg9->Draw("same");

  TLatex* tex10 = new TLatex(0.68,0.8,"2.71 fb^{-1} (13 TeV)");
  tex10->SetNDC(kTRUE);
  tex10->SetLineWidth(2);
  tex10->SetTextSize(0.04);
  tex10->Draw();
  tex10 = new TLatex(0.68,0.85,"CMS Preliminary");
  tex10->SetNDC(kTRUE);
  tex10->SetTextFont(42);
  tex10->SetTextSize(0.04);
  tex10->SetLineWidth(2);
  tex10->Draw();



  c9.SetLogy();
  if(mc==0) c9.SaveAs("lerrxy_sideband_sub.png");

  return histos;
}
