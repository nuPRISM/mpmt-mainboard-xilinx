{


  {


    int first_run = 300;
    int last_run = 304;
    int total_runs = last_run - first_run;
    
    TH1D *stack_ph[20];// = new TH1D("stack_ph","Pulse Heights vs Voltage",
    int hv[20];

    TGraphErrors *ph_vs_volt[20];
    for(int i = 0; i < 20; i++){ph_vs_volt[i] = new TGraphErrors();}
    
    char name[100];
    for(int run = first_run; run <= last_run; run++){

      sprintf(name,"output%08i.root",run);
      TFile *f = new TFile(name);

      // Get HV set points
      TTree *tree = (TTree*)f->Get("settings");
      int hv_set[20];
      tree->SetBranchAddress( "hv_set", hv_set );
      tree->GetEntry(0);
      std::cout << "open file " << name << " at HV= " << hv_set[0] << std::endl;
      hv[run-first_run] = hv_set[0];
      
      // Get pulse height histograms
      for(int ch = 0; ch < 1; ch++){
        sprintf(name,"BRB_PH_%i;1",ch);
        TH1D *tmp = (TH1D*)f->Get(name);
        stack_ph[run-first_run] = tmp;

        int max_bin = tmp->GetBinCenter(tmp->GetMaximumBin());
        tmp->Fit("gaus","Q","",max_bin - 7, max_bin + 5);
        TF1 *myfunc = tmp->GetFunction("gaus");
        std::cout << myfunc->GetParameter(0) << " "
                  << myfunc->GetParameter(1) << " " <<std::endl;

        int nn = ph_vs_volt[ch]->GetN();
        ph_vs_volt[ch]->SetPoint(nn,hv_set[0],myfunc->GetParameter(1));
        ph_vs_volt[ch]->SetPointError(nn,0,myfunc->GetParError(1));
          
      }
      
    }

    TCanvas *c = new TCanvas("C");
    TLegend *leg = new TLegend(0.1,0.1, 0.4,0.3);

    for(int i = total_runs; i >= 0; i--){
      if(i == total_runs){
        stack_ph[i]->Draw();
      }else{
        stack_ph[i]->Draw("SAME");
      }
      stack_ph[i]->SetLineColor(i+1);
      sprintf(name,"%iV",hv[i]);
      leg->AddEntry(stack_ph[i],name);
      
    }
    leg->Draw("SAME");

    TCanvas *c2 = new TCanvas("C2");
    ph_vs_volt[0]->Draw("AP*");
    ph_vs_volt[0]->SetMarkerStyle(20);
    ph_vs_volt[0]->GetXaxis()->SetTitle("Voltage (V)");    
    ph_vs_volt[0]->GetYaxis()->SetTitle("SPE Pulse Height (counts)");    


  }
}
