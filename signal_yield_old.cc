#include <TStyle.h>
#include <TAxis.h>
#include <TLatex.h>
#include <TPaveText.h>
#include <TFile.h>
#include <TTree.h>
#include <TString.h>
#include <TCanvas.h>
#include <TNtupleD.h>
#include <TH1D.h>
#include <TLorentzVector.h>
#include <RooRealVar.h>
#include <RooConstVar.h>
#include <RooDataSet.h>
#include <RooGaussian.h>
#include <RooChebychev.h>
#include <RooBernstein.h>
#include <RooExponential.h>
#include <RooAddPdf.h>
#include <RooPlot.h>
//#include "PhysicsTools/Utilities/interface/SideBandSubtraction.h"
#include "myloop.h"
#include "plotDressing.h"
#include "TMath.h"
using namespace RooFit;

// General fitting options
#define NUMBER_OF_CPU       1
#define YIELD_SUB_SAMPLES   0

#define VERSION             "v7"
//-----------------------------------------------------------------
// Definition of channel #
// channel = 1: B+ -> J/psi K+
// channel = 2: B0 -> J/psi K*
// channel = 3: B0 -> J/psi Ks
// channel = 4: Bs -> J/psi phi
// channel = 5: Jpsi + pipi
// channel = 6: Lambda_b -> Jpsi + Lambda

void plot_pt_dist(RooWorkspace& w, int channel, TString directory);
void plot_mass_fit(RooWorkspace& w, int channel, TString directory);

void fit_mass_full_dataset(RooWorkspace& w, int channel);
//void fit_mass_pt_bin(RooWorkspace& w, int channel, double& pt_bin_edges);

void build_pdf(RooWorkspace& w, int channel);
void read_data(RooWorkspace& w, TString filename,int channel);
void read_data_cut(RooWorkspace& w, RooDataSet* data);
void set_up_workspace_variables(RooWorkspace& w, int channel);

TString channel_to_ntuple_name(int channel);
TString channel_to_xaxis_title(int channel);
int channel_to_nbins(int channel);

void signal_yield_old(int channel)
{
  cout << "start of signal_yield"<< std::endl;

  double ntkp_pt_bin_edges[]={10,20,30,40,50,60,70,80,90,100,120};
  double ntkstar_pt_bin_edges[]={0};
  double ntks_pt_bin_edges[]={10,20,30,40,50,60,70};
  double ntphi_pt_bin_edges[]={10,15,20,25,30,35,40,45,50,55,60,70};
  double ntmix_pt_bin_edges[]={0};
  double ntlambda_pt_bin_edges[]={0};
  double* pt_bin_edges;
  
  int nptbins;
  
  TString data_selection_input_file = "selected_data_" + channel_to_ntuple_name(channel) + ".root";
  
  RooWorkspace* ws = new RooWorkspace("ws","Bmass");
  RooAbsData* data;
  RooAbsPdf* model;
  RooRealVar* signal_res;

  TString pt_dist_directory="";
  TString mass_fit_directory="";

  //set up mass and pt variables inside ws  
  set_up_workspace_variables(*ws,channel);

  //read data from the selected data file, and import it as a dataset into the workspace.
  read_data(*ws, data_selection_input_file,channel);
  
  if(!YIELD_SUB_SAMPLES) //mass fit and plot the full dataset
    { 
      fit_mass_full_dataset(*ws,channel);
  
      mass_fit_directory = "full_dataset_mass_fit/" + channel_to_ntuple_name(channel) + "_" + TString::Format(VERSION);
      plot_mass_fit(*ws,channel,mass_fit_directory);
    }
  else
    {
      switch (channel) {
      case 1:
	pt_bin_edges = ntkp_pt_bin_edges;
	nptbins = (sizeof(ntkp_pt_bin_edges) / sizeof(double)) - 1 ; //if pt_bin_edges is an empty array, then nptbins is equal to 0
	break;
      case 2:
	pt_bin_edges = ntkstar_pt_bin_edges;
	nptbins = (sizeof(ntkstar_pt_bin_edges) / sizeof(double)) - 1 ;
	break;
      case 3:
	pt_bin_edges = ntks_pt_bin_edges;
	nptbins = (sizeof(ntks_pt_bin_edges) / sizeof(double)) - 1 ;
	break;
      case 4:
	pt_bin_edges = ntphi_pt_bin_edges;
	nptbins = (sizeof(ntphi_pt_bin_edges) / sizeof(double)) - 1 ;
	break;
      case 5:
	pt_bin_edges = ntmix_pt_bin_edges;
	nptbins = (sizeof(ntmix_pt_bin_edges) / sizeof(double)) - 1 ;
	break;
      case 6:
	pt_bin_edges = ntlambda_pt_bin_edges;
	nptbins = (sizeof(ntlambda_pt_bin_edges) / sizeof(double)) - 1 ;
	break;
      }

      RooDataSet* data_original  = new RooDataSet("data_original", "data_original", *(ws->data("data")->get()),Import( *(dynamic_cast<RooDataSet *>(ws->data("data"))) ));
      // RooAbsData* data_original = ws->data("data");
      RooRealVar pt = *(ws->var("pt"));

      RooThresholdCategory ptRegion("ptRegion", "region of pt", pt);
      ptRegion.addThreshold(*(pt_bin_edges),"below 1st bin");

      for(int i=0; i<nptbins; i++)
	{
	  TString reg = TString::Format("PtBin%d",i+1);
	  ptRegion.addThreshold(*(pt_bin_edges+i+1),reg);
	}
      data_original->addColumn(ptRegion);
      
      Roo1DTable * tab = data_original->table(ptRegion);
      tab->Print("v");
      delete tab;

      //to produce and process each pt subsample                                                                                                    
      RooDataSet *data_cut;
      RooWorkspace* ws_cut;
      RooAbsPdf* model_cut;
      RooRealVar* pt_mean;
      TString directory="";
      double pt_bin_centre[nptbins];
      double pt_bin_edges_Lo[nptbins];
      double pt_bin_edges_Hi[nptbins];
      double yield_array[nptbins];
      double errLo_array[nptbins];
      double errHi_array[nptbins];

      for(int i=0; i<nptbins; i++)
	{
	  cout << "processing subsample pt: " << i+1 << std::endl;

	  TString ptcut(TString::Format("(ptRegion==ptRegion::PtBin%d)", i+1));

	  data_cut = new RooDataSet("data", "data", *(data_original->get()),Import(*data_original), Cut(ptcut));

	  // TString ptcut(TString::Format("(pt>(pt_bin_edges+%d))&&(pt<(pt_bin_edges+%d+1))",i));
	  //RooFormulaVar ptcut("pt_cut","pt_cut","pt>*(pt_bin_edges+i) && pt<*(pt_bin_edges+i+1)",RooArgList(pt_bin_edges,i));
	  //data_cut=data_original->reduce(Cut(ptcut));

	  ws_cut = new RooWorkspace("ws_cut","Bmass_cut");
	  set_up_workspace_variables(*ws_cut,channel);
	  read_data_cut(*ws_cut,data_cut);
	  build_pdf(*ws_cut,channel);

	  model_cut = ws_cut->pdf("model");
	  ws_cut->Print();

	  pt_mean = data_cut->meanVar(pt); //older way: pt_bin_centre[i] = *(pt_bin_edges+i) + (*(pt_bin_edges+i+1)-*(pt_bin_edges+i))/2;
	  pt_bin_centre[i] = (double) pt_mean->getVal();
	  pt_bin_edges_Lo[i] = pt_bin_centre[i] - *(pt_bin_edges+i);
	  pt_bin_edges_Hi[i] = *(pt_bin_edges+i+1) - pt_bin_centre[i];

	  fit_res = model_cut->fitTo(*data_cut,Minos(kTRUE),NumCPU(NUMBER_OF_CPU),Offset(kTRUE));
	  signal_res = ws_cut->var("n_signal");

	  yield_array[i] = signal_res->getVal();
	  errLo_array[i] = -signal_res->getAsymErrorLo();
	  errHi_array[i] = signal_res->getAsymErrorHi();

	  directory = "pt_bin_mass_fit/" + channel_to_ntuple_name(channel) + "_" + TString::Format(VERSION) + "/" + "mass_fit_" + channel_to_ntuple_name(channel) + TString::Format("_bin_%d_%d", (int)*(pt_bin_edges+i), (int)*(pt_bin_edges+i+1));
	    
	  plot_mass_fit(*ws_cut,channel,directory);
	    
	  //how to put the legend indicating each pt bin ??
	  //change the plot_mass_fit to output a TCanvas, and write the legend on top after, and then have a function just to save the plots.
	  //  Legend(channel,(int)pt_bin_lo,(int)pt_bin_hi,1);
	}

      for(int i=0; i<nptbins; i++)
	{
	  std::cout << "BIN: "<< (int) *(pt_bin_edges+i) << " to " << (int) *(pt_bin_edges+i+1) << " : " <<  yield_array[i] << " +" << errHi_array[i] << " -"<< errLo_array[i] << std::endl;
	}
      TCanvas cz;
      TGraphAsymmErrors* graph = new TGraphAsymmErrors(nptbins, pt_bin_centre, yield_array, pt_bin_edges_Lo, pt_bin_edges_Hi, errLo_array, errHi_array);
      graph->SetTitle("Raw signal yield in Pt bins");
      graph->SetFillColor(2);
      graph->SetFillStyle(3001);
      graph->Draw("a2");
      graph->Draw("p");
      cz.SetLogy();
      cz.SaveAs("signal_yield/signal_yield_" + channel_to_ntuple_name(channel) + "_" + TString::Format(VERSION) + ".root");
      cz.SaveAs("signal_yield/signal_yield_" + channel_to_ntuple_name(channel) + "_" + TString::Format(VERSION) + ".png");
    }
}

//void fit_mass_full_dataset(RooWorkspace& w, int channel)

void plot_mass_fit(RooWorkspace& w, int channel, TString directory)
{
  RooRealVar mass = *(w.var("mass"));
  RooAbsData* data = w.data("data");
  RooAbsPdf* model = w.pdf("model");
     
  RooPlot* frame_m = mass.frame();
    
  TH1D* histo_data = (TH1D*)data->createHistogram("histo_data", mass, Binning(channel_to_nbins(channel), mass.getMin(), mass.getMax() ));
  histo_data->Sumw2(false);
  histo_data->SetBinErrorOption(TH1::kPoisson);
  histo_data->SetMarkerStyle(20);
  histo_data->SetMarkerSize(0.8);
  histo_data->SetLineColor(kBlack);
  
  for (int i=1; i<=channel_to_nbins(channel); i++)
    if (histo_data->GetBinContent(i)==0) histo_data->SetBinError(i,0.);
  
  data->plotOn(frame_m,Name("theData"),Binning(channel_to_nbins(channel)),Invisible());
  
  model->plotOn(frame_m,Name("thePdf"),Precision(2E-4));
  
  //model->paramOn(frame_m); //show all the parameters of the fit in the plot.
  
  model->plotOn(frame_m,Precision(2E-4),Components("pdf_m_signal"),LineColor(kRed),LineWidth(2),LineStyle(kSolid),FillStyle(3008),FillColor(kRed), VLines(), DrawOption("F"));
  
  if (channel==1 || channel==2 || channel==3 || channel==4 || channel==6)
    model->plotOn(frame_m,Precision(2E-4),Components("pdf_m_combinatorial_exp"),LineColor(kCyan+1),LineWidth(2),LineStyle(2));
  else
    model->plotOn(frame_m,Precision(2E-4),Components("pdf_m_combinatorial_bern"),LineColor(kCyan+1),LineWidth(2),LineStyle(2));
  
  if (channel==1 || channel==3)
    model->plotOn(frame_m,Precision(2E-4),Components("pdf_m_jpsix"),LineColor(kViolet),LineWidth(2),LineStyle(7));
  if (channel==5)
    model->plotOn(frame_m,Precision(2E-4),Components("pdf_m_x3872"),LineColor(kOrange),LineWidth(2),LineStyle(kSolid),FillStyle(3008),FillColor(kOrange), VLines(), DrawOption("F"));
  
  frame_m->SetTitle("");
  frame_m->GetXaxis()->SetTitle(channel_to_xaxis_title(channel));
  frame_m->GetXaxis()->SetLabelFont(42);
  frame_m->GetXaxis()->SetLabelOffset(0.01);
  frame_m->GetXaxis()->SetTitleSize(0.06);
  frame_m->GetXaxis()->SetTitleOffset(1.09);
  frame_m->GetXaxis()->SetLabelFont(42);
  frame_m->GetXaxis()->SetLabelSize(0.055);
  frame_m->GetXaxis()->SetTitleFont(42);
  frame_m->GetYaxis()->SetTitle(TString::Format("Events / %g MeV",(mass.getMax()-mass.getMin())*1000./channel_to_nbins(channel)));
  frame_m->GetYaxis()->SetLabelFont(42);
  frame_m->GetYaxis()->SetLabelOffset(0.01);
  frame_m->GetYaxis()->SetTitleOffset(1.6);
  frame_m->GetYaxis()->SetTitleSize(0.05);
  frame_m->GetYaxis()->SetTitleFont(42);
  frame_m->GetYaxis()->SetLabelFont(42);
  frame_m->GetYaxis()->SetLabelSize(0.055);
  
  RooHist* pull_hist = frame_m->pullHist("theData","thePdf");
  
  RooPlot* pull_plot = mass.frame();
  pull_plot->addPlotable(pull_hist,"P");
  pull_plot->SetTitle("");
  pull_plot->GetXaxis()->SetTitle(channel_to_xaxis_title(channel));
  pull_plot->GetXaxis()->SetLabelFont(42);
  pull_plot->GetXaxis()->SetLabelOffset(0.01);
  pull_plot->GetXaxis()->SetTitleSize(0.06);
  pull_plot->GetXaxis()->SetTitleOffset(1.09);
  pull_plot->GetXaxis()->SetLabelFont(42);
  pull_plot->GetXaxis()->SetLabelSize(0.055);
  pull_plot->GetXaxis()->SetTitleFont(42);
  pull_plot->GetYaxis()->SetTitle(TString::Format("Events / %g MeV",(mass.getMax()-mass.getMin())*1000./channel_to_nbins(channel)));
  pull_plot->GetYaxis()->SetLabelFont(42);
  pull_plot->GetYaxis()->SetLabelOffset(0.01);
  pull_plot->GetYaxis()->SetTitleOffset(1.14);
  pull_plot->GetYaxis()->SetTitleSize(0.06);
  pull_plot->GetYaxis()->SetTitleFont(42);
  pull_plot->GetYaxis()->SetLabelFont(42);
  pull_plot->GetYaxis()->SetLabelSize(0.055);

  TCanvas *c1 = canvasDressing("c1"); c1->cd();
  
  // TPad *p1 = new TPad("p1","p1",0,0,1,1);
  //p1->Draw();
   
  //  TPad *p1 = new TPad("p1","p1",0.03,0.27,0.99,0.99);
  TPad *p1 = new TPad("p1","p1",0.05,0.05,0.99,0.99);
  p1->SetBorderMode(0); 
  p1->Draw(); 
  
  /*
  TPad *p2 = new TPad("p2","p2",0.03,0.01,0.99,0.26); 
  p2->SetTopMargin(0.);    
  p2->SetBorderMode(0); 
  p2->SetTicks(1,2); 
  p2->Draw();
  */
  p1->cd(); frame_m->Draw(); histo_data->Draw("Esame"); Legend(channel,0,0,0);
  //p2->cd(); pull_plot->Draw();
  
  c1->SaveAs(directory + ".root");
  c1->SaveAs(directory + ".png"); 
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

void build_pdf(RooWorkspace& w, int channel)
{
  double mass_peak;

  RooRealVar mass = *(w.var("mass"));
  RooRealVar pt = *(w.var("pt"));  
  RooAbsData* data = w.data("data");

  switch (channel) {
  case 1:
    mass_peak = BP_MASS;
    break;
  case 2:
    mass_peak = B0_MASS;
    break;
  case 3:
    mass_peak = B0_MASS;
    break;
  case 4:
    mass_peak = BS_MASS;
    break;
  case 5:
    mass_peak = PSI2S_MASS;
    break;
  case 6:
    mass_peak = LAMBDAB_MASS;
    break;
  }
  
  double n_signal_initial = data->sumEntries(TString::Format("abs(mass-%g)<0.015",mass_peak))
    - data->sumEntries(TString::Format("abs(mass-%g)<0.030&&abs(mass-%g)>0.015",mass_peak,mass_peak));
  
  double n_combinatorial_initial = data->sumEntries() - n_signal_initial;
  
  //-----------------------------------------------------------------
  // signal PDF 
  RooRealVar m_mean("m_mean","m_mean",mass_peak,mass.getMin(),mass.getMax());
  RooRealVar m_sigma1("m_sigma1","m_sigma1",0.015,0.001,0.050);
  RooRealVar m_sigma2("m_sigma2","m_sigma2",0.030,0.001,0.100);
  RooRealVar m_fraction("m_fraction","m_fraction",0.5);
  RooGaussian m_gaussian1("m_gaussian1","m_gaussian1",mass,m_mean,m_sigma1);
  RooGaussian m_gaussian2("m_gaussian2","m_gaussian2",mass,m_mean,m_sigma2);
  RooAddPdf pdf_m_signal("pdf_m_signal","pdf_m_signal",RooArgList(m_gaussian1,m_gaussian2),RooArgList(m_fraction));
  
  // use single Gaussian for J/psi Ks and J/psi Lambda due to low statistics
  if (channel==3 || channel==6) {
    m_sigma2.setConstant(kTRUE);
    m_fraction.setVal(1.);
  }
  
  //-----------------------------------------------------------------
  // combinatorial background PDF (exponential or bernstean poly.)
  
  RooRealVar m_exp("m_exp","m_exp",-0.3,-4.,+4.);
  RooExponential pdf_m_combinatorial_exp("pdf_m_combinatorial_exp","pdf_m_combinatorial_exp",mass,m_exp);
  
  RooRealVar m_par1("m_par1","m_par2",1.,0,+10.);
  RooRealVar m_par2("m_par2","m_par3",1.,0,+10.);
  RooRealVar m_par3("m_par3","m_par3",1.,0,+10.);
  
  RooBernstein pdf_m_combinatorial_bern("pdf_m_combinatorial_bern","pdf_m_combinatorial_bern",mass,RooArgList(RooConst(1.),m_par1,m_par2,m_par3));
  //erfc component on channel 1 and 3
  RooFormulaVar pdf_m_jpsix("pdf_m_jpsix","2.7*erfc((mass-5.14)/(0.5*0.08))",{mass});
  
  //-----------------------------------------------------------------
  // X(3872) PDF, only for J/psi pipi fit
  
  RooRealVar m_x3872_mean("m_x3872_mean","m_x3872_mean",3.872,3.7,3.9);
  RooRealVar m_x3872_sigma("m_x3872_sigma","m_x3872_sigma",0.01,0.001,0.010);
  RooGaussian pdf_m_x3872("pdf_m_x3872","pdf_m_x3872",mass,m_x3872_mean,m_x3872_sigma);
  
  //-----------------------------------------------------------------
  // full model
  
  RooRealVar n_signal("n_signal","n_signal",n_signal_initial,0.,data->sumEntries());
  RooRealVar n_combinatorial("n_combinatorial","n_combinatorial",n_combinatorial_initial,0.,data->sumEntries());
  RooRealVar n_x3872("n_x3872","n_x3872",200.,0.,data->sumEntries());
  
  RooRealVar n_jpsix("n_jpsix","n_jpsix",data->sumEntries(TString::Format("mass>4.9&&mass<5.14")),data->sumEntries(TString::Format("mass>4.9&&mass<5.14")),data->sumEntries());
  
  RooAddPdf* model;
  
  if (channel==1 || channel ==3) // B+ -> J/psi K+, B0 -> J/psi Ks
    model = new RooAddPdf("model","model",
			  RooArgList(pdf_m_signal, pdf_m_combinatorial_exp, pdf_m_jpsix),
			  RooArgList(n_signal, n_combinatorial, n_jpsix));
  else
    if (channel==2 || channel==4 || channel==6) // B0 -> J/psi K*; Bs -> J/psi phi; Lambda_b -> J/psi Lambda
      model = new RooAddPdf("model","model",
			    RooArgList(pdf_m_signal, pdf_m_combinatorial_exp),
			    RooArgList(n_signal, n_combinatorial));
    else
      if (channel==5) // J/psi pipi
	model = new RooAddPdf("model","model",
			      RooArgList(pdf_m_signal, pdf_m_combinatorial_bern, pdf_m_x3872),
			      RooArgList(n_signal, n_combinatorial, n_x3872));

  w.import(*model);
}

void read_data(RooWorkspace& w, TString filename,int channel)
{
  TFile* f = new TFile(filename);
  TNtupleD* _nt = (TNtupleD*)f->Get(channel_to_ntuple_name(channel));
 
  RooDataSet* data = new RooDataSet("data","data",_nt,RooArgSet( *(w.var("mass")) , *(w.var("pt")) ));
  
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

  pt_min=0;
  pt_max=400;
  
  switch (channel) {
  case 1:
    mass_min = 5.0; mass_max = 6.0;
    break;
  case 2:
    mass_min = 5.0; mass_max = 6.0;
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

  w.import(mass);
  w.import(pt);
}

TString channel_to_ntuple_name(int channel)
{
  //returns a TString with the ntuple name corresponding to the channel. It can be used to find the data on each channel saved in a file. or to write the name of a directory

  TString ntuple_name = "";

  switch(channel){
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

TString channel_to_xaxis_title(int channel)
{
  TString xaxis_title = "";

  switch (channel) {
  case 1:
    xaxis_title = "M_{J/#psi K^{#pm}} [GeV]";
    break;
  case 2:
    xaxis_title = "M_{J/#psi K^{#pm}#pi^{#mp}} [GeV]";
    break;
  case 3:
    xaxis_title = "M_{J/#psi K^{0}_{S}} [GeV]";
    break;
  case 4:
    xaxis_title = "M_{J/#psi K^{#pm}K^{#mp}} [GeV]";
    break;
  case 5:
    xaxis_title = "M_{J/#psi #pi^{#pm}#pi^{#mp}} [GeV]";
    break;
  case 6:
    xaxis_title = "M_{J/#psi #Lambda} [GeV]";
    break;
  }
  return xaxis_title;
}

int channel_to_nbins(int channel)
{
  int nbins;

  switch (channel) {
  case 1:
    nbins = 50;
    break;
  case 2:
    nbins = 50;
    break;
  case 3:
    nbins = 50;
    break;
  case 4:
    nbins = 50;
    break;
  case 5:
    nbins = 80;
    break;
  case 6:
    nbins = 50;
    break;
  }
  return nbins;
}
