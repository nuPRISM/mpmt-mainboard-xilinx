{


  {


    int first_run = 305;
    int last_run = 324;

    int all_runs = 0;
    TH1D *stack_ph[20][20];// = new TH1D("stack_ph","Pulse Heights vs Voltage",
    int hv[20];

    TGraphErrors *ph_vs_volt[20];
    for(int i = 0; i < 20; i++){
        ph_vs_volt[i] = new TGraphErrors();
    }
    
    char name[100];
    for(int run = first_run; run <= last_run; run++){

      sprintf(name,"output%08i.root",run);
      TFile *f = new TFile(name);

      if(!f) continue;
      // Get HV set points
      TTree *tree = (TTree*)f->Get("settings");
      if(!tree) continue;
      int hv_set[20];
      tree->SetBranchAddress( "hv_set", hv_set );
      tree->GetEntry(0);
      std::cout << "open file " << name << " at HV= " << hv_set[0] << std::endl;
      hv[all_runs] = hv_set[0];
      
      // Get pulse height histograms
      for(int i = 0; i < 10; i++){
        int ch = i;
        if(i > 1) ch = i + 2;
        
        sprintf(name,"BRB_PH_%i;1",ch);
        TH1D *tmp = (TH1D*)f->Get(name);
        stack_ph[i][all_runs] = tmp;

        //
        int max_bin = 8;
        int max_value = 0;
        for(int i = 11; i < 40; i++){
          if(tmp->GetBinContent(i) > max_value){
            max_value = (tmp->GetBinContent(i));
            max_bin = tmp->GetBinCenter(i);
                         
          }
        }
        
        //        int max_bin = tmp->GetBinCenter(tmp->GetMaximumBin());
        float range = 5;
        if(max_bin > 20) range = 10;
        if(max_bin > 30) range = 15;
        if(tmp->GetEntries() < 50) continue; // don't bother
        tmp->Fit("gaus","Q","",max_bin - range, max_bin + range);
        TF1 *myfunc = tmp->GetFunction("gaus");
        std::cout << i << " " << ch << " "
                  << myfunc->GetParameter(0) << " "
                  << myfunc->GetParameter(1) << " " <<std::endl;

        int nn = ph_vs_volt[i]->GetN();
        ph_vs_volt[i]->SetPoint(nn,hv_set[0],myfunc->GetParameter(1));
        ph_vs_volt[i]->SetPointError(nn,0,myfunc->GetParError(1));
          
      }

      all_runs++;
    }

    std::cout << "Total number of runs: " << all_runs << std::endl;


    for(int j = 0; j < 9; j++){

      char name[100];
      sprintf(name,"C%i",j);
      
      TCanvas *c = new TCanvas(name);
      TLegend *leg = new TLegend(0.1,0.1, 0.4,0.3);
      
      for(int i = all_runs-1; i >= 0; i--){
        
        //if(hv[i] % 50 != 0) continue;
        
        if(i == all_runs-1){
          stack_ph[j][i]->Draw();
        }else{
          stack_ph[j][i]->Draw("SAME");
        }
        stack_ph[j][i]->SetLineColor(i+1);
        stack_ph[j][i]->GetFunction("gaus")->SetLineColor(i+1);
        sprintf(name,"%iV",hv[i]);
        leg->AddEntry(stack_ph[j][i],name);
        
      }
      leg->Draw("SAME");

    }

    TCanvas *c2 = new TCanvas("Cvolt");
    TLegend *leg2 = new TLegend(0.1,0.6,0.4,0.89);
    for(int i = 0; i < 9; i++){
      char name[100];
      if(i < 2){ sprintf(name,"Channel %i",i); }
      else{ sprintf(name,"Channel %i",i+2); }
      
      
      if(i == 0){
        ph_vs_volt[i]->Draw("AP*L");
        ph_vs_volt[i]->SetMarkerStyle(20);
        ph_vs_volt[i]->GetXaxis()->SetTitle("Voltage (V)");    
        ph_vs_volt[i]->GetYaxis()->SetTitle("SPE Pulse Height (counts)");    
      }else{
        ph_vs_volt[i]->Draw("*L");
        ph_vs_volt[i]->SetMarkerStyle(20+i);
        ph_vs_volt[i]->SetLineColor(1+i);
        ph_vs_volt[i]->SetMarkerColor(1+i);
        if(i == 9){
        ph_vs_volt[i]->SetLineColor(2);
        ph_vs_volt[i]->SetMarkerColor(2);


        }

      }
      leg2->AddEntry(ph_vs_volt[i],name,"P");

    }
    leg2->Draw("SAME");

  }
}
