{


  {


    int first_run = 442;
    int last_run = 452;

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
      for(int i = 0; i < 12; i++){
        int ch = i+4;        
        //if(i > 7) ch = i + 1;
        
        sprintf(name,"BRB_PH_%i;1",ch);
        TH1D *tmp = (TH1D*)f->Get(name);
        stack_ph[i][all_runs] = tmp;

        //
        int max_bin = 8;
        int max_value = 0;
        int start_bin = 11;
        if(tmp->GetMean() > 20) start_bin = 20;
        for(int i = start_bin; i < 40; i++){
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
        
        if(hv[i] % 50 != 0) continue;
        
        if(i == all_runs-1){
          stack_ph[j][i]->Draw();
        }else{
          stack_ph[j][i]->Draw("SAME");
        }
        stack_ph[j][i]->SetLineColor(i);
        stack_ph[j][i]->GetFunction("gaus")->SetLineColor(i);
        sprintf(name,"%iV",hv[i]);
        leg->AddEntry(stack_ph[j][i],name);
        
      }
      leg->Draw("SAME");

    }

    std::cout << "Full plot " << std::endl;
    TCanvas *c2 = new TCanvas("Cvolt");
    TLegend *leg2 = new TLegend(0.1,0.6,0.4,0.89);
    for(int i = 0; i < 12; i++){
      char name[100];

      int ch = i+4; 


      sprintf(name,"Channel %i",ch); 

      
      
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


      // calculate operating HV
      double target_ph = 16.0;
      double x1,y1;
      double x2,y2;
      for(int j = 0; j < ph_vs_volt[i]->GetN() - 1; j++){
        
        ph_vs_volt[i]->GetPoint(j, x1, y1);
        ph_vs_volt[i]->GetPoint(j+1, x2, y2);
        if(y1 < target_ph && y2 > target_ph){
          double b=y1;
          double m=(y2-y1)/(x2-x1);
	  
          // y = m * (x-x1) + b
          // (y - b)/m + x1 = x
          double set_point = (target_ph - b)/m + x1;

          std::cout << " Ch  " << ch  
                    << " setpoint=" << set_point 
                    << std::endl;

          
          break;
        }

      }
      
    }
    leg2->Draw("SAME");

  }
}
