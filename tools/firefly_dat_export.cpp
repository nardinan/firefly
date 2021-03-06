/*
 * firefly
 * Copyright (C) 2013 Andrea Nardinocchi (andrea@nardinan.it)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "../root_analyzer.h"
/* from the Matteo 'Bozzochet' Duranti's API */
#include "Cluster.hh"
#include "Event.hh"
/* end */
#define d_cuts_steps 5
#define d_sqrt_mip 7.0711
#define d_AMS_ladder_one 2
#define d_AMS_ladder_two 3
#define d_AMS_sqrt_mip 5.9161
#define d_AMS_signal_cut 4.0
#define d_AMS_channels 640
int v_correlation = false, v_last_root = 0, v_shift_dampe = 0, v_shift_ams = 0;
TFile *v_stream;
TTree *v_branch;
Event *v_current_event;
RHClass *v_rhclass;
Long64_t v_entries;
typedef struct s_data_cuts {
  float low, top;
} s_data_cuts;
struct s_data_cuts cuts[d_cuts_steps] = {
  {18.0, 30.0},
  {30.0, 50.0},
  {50.0, 70.0},
  {70.0, 90.0},
  {90.0, 110.0}
};
typedef enum e_correlation_ams {
  e_correlation_ams_DAMPE1_AMS1 = 0,
  e_correlation_ams_DAMPE2_AMS1,
  e_correlation_ams_DAMPE1_AMS2,
  e_correlation_ams_DAMPE2_AMS2,
  e_correlation_ams_AMS1_AMS2,
  e_correlation_ams_NULL
} e_correlation_ams;
typedef struct s_data_charts {
  TH1F *n_channels, *n_clusters, *common_noise, *signals, *signals_MIP, *signals_array[5], *signal_over_noise, *strips_gravity, *main_strips_gravity, 
       *eta, *eta_array[5], *channel_one, *channels_two, *channels_two_major, *channels_two_minor, *signal_one, *signals_two, *signals_two_major, 
       *signals_two_minor, *etas[d_cuts_steps];
  TH2F *signal_eta, *signal_gravity, *signal_gravity_MIP, *n_channels_gravity, *correlation, *correlation_signal, 
       *correlation_signal_AMS[e_correlation_ams_NULL], *correlation_ams;
  TH1D *profile, *profile_sg, *profile_sgm, *profile_nc_g;
} s_data_charts;
void f_load_ams(struct o_string *data) {
  v_stream = new TFile(data->content);
  if (v_stream->IsOpen())
    if ((v_branch = (TTree *)(v_stream->Get("t4"))))
      if ((v_entries = v_branch->GetEntries()) > 0) {
        v_current_event = new Event();
        v_branch->SetBranchAddress("cluster_branch", &v_current_event);
        v_branch->GetEntry(0);
        v_rhclass = (RHClass *)v_branch->GetUserInfo()->At(0);
      }
}

void f_fill_histograms(struct o_string *data, struct s_data_charts *charts) {
  struct o_stream *stream;
  struct s_singleton_file_header file_header;
  struct s_singleton_event_header event_header;
  struct s_singleton_cluster_details *clusters;
  struct s_exception *exception = NULL;
  float value, value_A, value_B;
  int index, subindex, strip, current_strip, current_event = 0;
  bool founded; 
  Cluster *ams_cluster, *ams_subcluster;
  d_try {
    stream = f_stream_new_file(NULL, data, "rb", 0777);
    if ((stream->m_read_raw(stream, (unsigned char *)&(file_header), sizeof(struct s_singleton_file_header)))) {
      if (file_header.endian_check == (unsigned short)d_compress_endian) {
        while ((clusters = f_decompress_event(stream, &event_header))) {
          printf("\r%80s\r[processing event: %d] %d clusters", "", current_event++, event_header.clusters);
          fflush(stdout);
          if (charts->n_clusters)
            charts->n_clusters->Fill(event_header.clusters);
          if (v_correlation) {
            /* retrieve AMS event ID */
            founded = false;
            if (((int)event_header.number-v_shift_dampe) >= 0)
              for (; v_last_root < v_entries; ++v_last_root) {
                v_branch->GetEntry(v_last_root);
                if ((v_current_event->Evtnum-v_shift_ams) > 0) {
                  if ((v_current_event->Evtnum-v_shift_ams) == ((int)event_header.number-v_shift_dampe)) {
                    founded = true;
                    break;
                  } else if ((v_current_event->Evtnum-v_shift_ams) > ((int)event_header.number-v_shift_dampe))
                    break;
                }
              }
            /* --- Porchetta time! */
            /* --- (brother Illo, please, forgive me) */
            if (founded) {
              for (index = 0; index < v_current_event->NClusTot; ++index)
                if ((ams_cluster = v_current_event->GetCluster(index)) && (ams_cluster->side == 1) &&
                    ((sqrt(ams_cluster->GetTotSig())/d_AMS_sqrt_mip) > d_AMS_signal_cut)) {
                  /* AMS #1 */
                  if (ams_cluster->ladder == d_AMS_ladder_one)
                    for (subindex = 0; subindex < v_current_event->NClusTot; ++subindex)
                      if ((ams_subcluster = v_current_event->GetCluster(subindex)) &&
                          (ams_subcluster->side == 1) &&
                          ((sqrt(ams_subcluster->GetTotSig())/d_AMS_sqrt_mip) > d_AMS_signal_cut)) {
                        /* AMS #2 */
                        if (ams_subcluster->ladder == d_AMS_ladder_two) 
                          charts->correlation_signal_AMS[e_correlation_ams_AMS1_AMS2]->Fill(
                              (sqrt(ams_cluster->GetTotSig())/d_AMS_sqrt_mip),
                              (sqrt(ams_subcluster->GetTotSig())/d_AMS_sqrt_mip));
                      }
                }
              for (index = 0; index < event_header.clusters; ++index) {
                if (clusters[index].first_strip < d_trb_event_channels) {
                  /* DAMPE #1 */
                  for (strip = 0, value_A = 0; strip < clusters[index].header.strips; strip++)
                    value_A += clusters[index].values[strip];
                  for (subindex = 0; subindex < v_current_event->NClusTot; ++subindex) 
                    if (((ams_cluster = v_current_event->GetCluster(subindex))) && 
                        ((sqrt(ams_cluster->GetTotSig())/d_AMS_sqrt_mip) > d_AMS_signal_cut)) {
                      if (ams_cluster->side == 0) {
                        /* AMS #1 */
                        if (ams_cluster->ladder == d_AMS_ladder_one)
                          charts->correlation_ams->Fill(clusters[index].header.strips_gravity,
                              ams_cluster->GetCoG());
                      } else if (ams_cluster->side == 1) {
                        /* AMS #1 */
                        if (ams_cluster->ladder == d_AMS_ladder_one)
                          charts->correlation_signal_AMS[e_correlation_ams_DAMPE1_AMS1]->
                            Fill(sqrt(value_A)/d_sqrt_mip, 
                                sqrt(ams_cluster->GetTotSig())/d_AMS_sqrt_mip);
                        /* AMS #2 */
                        if (ams_cluster->ladder == d_AMS_ladder_two)
                          charts->correlation_signal_AMS[e_correlation_ams_DAMPE1_AMS2]->
                            Fill(sqrt(value_A)/d_sqrt_mip, 
                                sqrt(ams_cluster->GetTotSig())/d_AMS_sqrt_mip);
                      }
                    }
                } else if (clusters[index].first_strip >= d_trb_event_channels) {
                  /* DAMPE #2 */
                  for (strip = 0, value_B = 0; strip < clusters[index].header.strips; strip++)
                    value_B += clusters[index].values[strip];
                  for (subindex = 0; subindex < v_current_event->NClusTot; ++subindex) 
                    if ((ams_cluster = v_current_event->GetCluster(subindex)) && (ams_cluster->side == 1) &&
                        ((sqrt(ams_cluster->GetTotSig())/d_AMS_sqrt_mip) > d_AMS_signal_cut)) {
                      /* AMS #1 */
                      if (ams_cluster->ladder == d_AMS_ladder_one)
                        charts->correlation_signal_AMS[e_correlation_ams_DAMPE2_AMS1]->
                          Fill(sqrt(value_B)/d_sqrt_mip, 
                              sqrt(ams_cluster->GetTotSig())/d_AMS_sqrt_mip);
                      /* AMS #2 */
                      if (ams_cluster->ladder == d_AMS_ladder_two)
                        charts->correlation_signal_AMS[e_correlation_ams_DAMPE2_AMS2]->
                          Fill(sqrt(value_B)/d_sqrt_mip, 
                              sqrt(ams_cluster->GetTotSig())/d_AMS_sqrt_mip);
                    }
                }
              }
            }
            for (index = 0; index < event_header.clusters; index++)
              if (clusters[index].first_strip < d_trb_event_channels) {
                /* cluster DAMPE 1 */
                for (strip = 0, value_A = 0; strip < clusters[index].header.strips; strip++)
                  value_A += clusters[index].values[strip];
                for (subindex = 0; subindex < event_header.clusters; ++subindex)
                  if (clusters[subindex].first_strip >= d_trb_event_channels) {
                    /* cluster DAMPE 2 */
                    charts->correlation->Fill(clusters[index].header.strips_gravity, 
                        clusters[subindex].header.strips_gravity);
                    for (strip = 0, value_B = 0; strip < clusters[subindex].header.strips; strip++)
                      value_B += clusters[subindex].values[strip];
                    charts->correlation_signal->Fill(sqrt(value_A)/d_sqrt_mip, 
                        sqrt(value_B)/d_sqrt_mip);
                  }
              }
          }
          for (index = 0; index < event_header.clusters; index++) {
            if (charts->signals) {
              for (strip = 0, value = 0, current_strip = clusters[index].first_strip;
                  strip < clusters[index].header.strips; strip++, current_strip++)
                value += clusters[index].values[strip];
              charts->signals->Fill(value);
              charts->signals_MIP->Fill(sqrt(value)/d_sqrt_mip);
              if (clusters[index].header.strips > 1)
                if (clusters[index].header.eta >= 0) {
                  charts->signal_eta->Fill(clusters[index].header.eta, value);
                  for (subindex = 0; subindex < d_cuts_steps; subindex++)
                    if ((value > cuts[subindex].low) && (value < cuts[subindex].top)) {
                      charts->etas[subindex]->Fill(clusters[index].header.eta);
                      break;
                    }
                }
              charts->signal_gravity->Fill(clusters[index].header.strips_gravity, value);
              charts->signal_gravity_MIP->Fill(clusters[index].header.strips_gravity, sqrt(value)/d_sqrt_mip);
              if ((clusters[index].header.strips >= 1) && (clusters[index].header.strips <= 5))
                charts->signals_array[clusters[index].header.strips-1]->Fill(value);
            }
            if (charts->n_channels)
              charts->n_channels->Fill(clusters[index].header.strips);
            charts->n_channels_gravity->Fill(clusters[index].header.strips_gravity, clusters[index].header.strips);
            if (charts->common_noise)
              charts->common_noise->Fill(clusters[index].values[clusters[index].header.strips]);
            if (charts->signal_over_noise)
              charts->signal_over_noise->Fill(clusters[index].header.signal_over_noise);
            if ((charts->channel_one) && (charts->channels_two) && (charts->channels_two_major) &&
                (charts->channels_two_minor)) {
              if (clusters[index].header.strips == 1) {
                charts->channel_one->Fill(clusters[index].header.signal_over_noise);
                if (charts->signal_one)
                  charts->signal_one->Fill(clusters[index].values[0]);
              } else if (clusters[index].header.strips == 2) {
                charts->channels_two->Fill(clusters[index].header.signal_over_noise);
                if (charts->signals_two)
                  charts->signals_two->Fill(clusters[index].values[0]+clusters[index].values[1]);
                current_strip = clusters[index].first_strip;
                if (clusters[index].values[0] > clusters[index].values[1]) {
                  charts->channels_two_major->Fill(clusters[index].values[0]/
                      file_header.sigma[current_strip]);
                  charts->channels_two_minor->Fill(clusters[index].values[1]/
                      file_header.sigma[current_strip+1]);
                  if ((charts->signals_two_major) && (charts->signals_two_minor)) {
                    charts->signals_two_major->Fill(clusters[index].values[0]);
                    charts->signals_two_minor->Fill(clusters[index].values[1]);
                  }
                } else {
                  charts->channels_two_major->Fill(clusters[index].values[1]/
                      file_header.sigma[current_strip+1]);
                  charts->channels_two_minor->Fill(clusters[index].values[0]/
                      file_header.sigma[current_strip]);
                  if ((charts->signals_two_major) && (charts->signals_two_minor)) {
                    charts->signals_two_major->Fill(clusters[index].values[1]);
                    charts->signals_two_minor->Fill(clusters[index].values[0]);
                  }
                }
              }
            }
            if (charts->strips_gravity)
              charts->strips_gravity->Fill(clusters[index].header.strips_gravity);
            if (charts->main_strips_gravity)
              charts->main_strips_gravity->Fill(clusters[index].header.main_strips_gravity);
            if (charts->eta)
              if (clusters[index].header.strips > 1)
                //if (charts->eta >= 0) {
                charts->eta->Fill(clusters[index].header.eta);
            if ((clusters[index].header.strips >= 1) && (clusters[index].header.strips <= 5))
              charts->eta_array[clusters[index].header.strips-1]->
                Fill(clusters[index].header.eta);
            //}
          }
          d_free(clusters);
        }
        printf("\n[done]\n");
      } else
        d_log(e_log_level_ever, "Wrong file format. Maybe this isn't a compressed data file ...\n");
    }
    d_release(stream);
  } d_catch(exception) {
    d_exception_dump(stderr, exception);
    d_raise;
  } d_endtry;
}

void f_export_histograms(struct o_string *output, struct s_data_charts *charts) {
  p_export_histograms_singleton(output, d_true, d_false, d_false, e_pdf_page_first, "HIST", "T", charts->n_clusters);
  p_export_histograms_singleton(output, d_true, d_false, d_false, e_pdf_page_middle, "HIST", "T", charts->n_channels);
  //p_export_histograms_singleton(output, d_true, d_false, e_pdf_page_middle, "HIST", "T", charts->common_noise);
  p_export_histograms_singleton(output, d_true, d_false, d_false, e_pdf_page_middle, "HIST", "T", charts->signals);
  p_export_histograms_singleton(output, d_true, d_false, d_false, e_pdf_page_middle, "HIST", "T", charts->signals_MIP);
  if (v_correlation) {
    p_export_histograms_singleton(output, d_false, d_true, d_false, e_pdf_page_middle, "COLZ", "T", charts->correlation);
    p_export_histograms_singleton(output, d_false, d_true, d_false, e_pdf_page_middle, "COLZ", "T", charts->correlation_signal);
    if (v_entries > 0) {
      p_export_histograms_singleton(output, d_false, d_true, d_false, e_pdf_page_middle, "COLZ", "T", charts->correlation_ams);
      p_export_histograms_singleton(output, d_false, d_true, d_false, e_pdf_page_middle, "COLZ", "T", charts->correlation_signal_AMS[0]);
      p_export_histograms_singleton(output, d_false, d_true, d_false, e_pdf_page_middle, "COLZ", "T", charts->correlation_signal_AMS[1]);
      p_export_histograms_singleton(output, d_false, d_true, d_false, e_pdf_page_middle, "COLZ", "T", charts->correlation_signal_AMS[2]);
      p_export_histograms_singleton(output, d_false, d_true, d_false, e_pdf_page_middle, "COLZ", "T", charts->correlation_signal_AMS[3]);
      p_export_histograms_singleton(output, d_false, d_true, d_false, e_pdf_page_middle, "COLZ", "T", charts->correlation_signal_AMS[4]);
    }

  }
  //p_export_histograms_singleton(output, d_true, d_false, e_pdf_page_middle, "HIST", "TTTTTT", charts->signals, charts->signals_array[0],
  //		charts->signals_array[1], charts->signals_array[2], charts->signals_array[3], charts->signals_array[4]);
  //p_export_histograms_singleton(output, d_true, d_false, e_pdf_page_middle, "HIST", "T", charts->signal_one);
  //p_export_histograms_singleton(output, d_true, d_false, e_pdf_page_middle, "HIST", "T", charts->signals_two);
  //p_export_histograms_singleton(output, d_true, d_false, e_pdf_page_middle, "HIST", "T", charts->signals_two_major);
  //p_export_histograms_singleton(output, d_true, d_false, e_pdf_page_middle, "HIST", "T", charts->signals_two_minor);
  //p_export_histograms_singleton(output, d_true, d_false, e_pdf_page_middle, "HIST", "T", charts->signal_over_noise);
  //p_export_histograms_singleton(output, d_true, d_false, e_pdf_page_middle, "HIST", "T", charts->channel_one);
  //p_export_histograms_singleton(output, d_true, d_false, e_pdf_page_middle, "HIST", "T", charts->channels_two);
  //p_export_histograms_singleton(output, d_true, d_false, e_pdf_page_middle, "HIST", "T", charts->channels_two_major);
  //p_export_histograms_singleton(output, d_true, d_false, e_pdf_page_middle, "HIST", "T", charts->channels_two_minor);
  p_export_histograms_singleton(output, d_false, d_false, d_false, e_pdf_page_middle, "HIST", "T", charts->strips_gravity);
  //p_export_histograms_singleton(output, d_false, d_false, e_pdf_page_middle, "HIST", "T", charts->main_strips_gravity);
  //p_export_histograms_singleton(output, d_false, d_false, e_pdf_page_middle, "HIST", "T", charts->eta);
  //p_export_histograms_singleton(output, d_false, d_false, e_pdf_page_middle, NULL, "T", charts->signal_eta);
  //p_export_histograms_singleton(output, d_false, d_false, e_pdf_page_middle, NULL, "T", charts->profile);
  p_export_histograms_singleton(output, d_false, d_false, d_false, e_pdf_page_middle, NULL, "T", charts->signal_gravity);
  p_export_histograms_singleton(output, d_false, d_false, d_false, e_pdf_page_middle, NULL, "T", charts->profile_sg);
  p_export_histograms_singleton(output, d_false, d_false, d_false, e_pdf_page_middle, NULL, "T", charts->signal_gravity_MIP);
  p_export_histograms_singleton(output, d_false, d_false, d_false, e_pdf_page_middle, NULL, "T", charts->profile_sgm);
  p_export_histograms_singleton(output, d_false, d_false, d_false, e_pdf_page_middle, NULL, "T", charts->n_channels_gravity);
  p_export_histograms_singleton(output, d_false, d_false, d_false, e_pdf_page_last, NULL, "T", charts->profile_nc_g);
  //p_export_histograms_singleton(output, d_false, d_false, e_pdf_page_middle, "HIST", "TTTTTT", charts->eta, charts->eta_array[0], charts->eta_array[1],
  //		charts->eta_array[2], charts->eta_array[3], charts->eta_array[4]);
  //p_export_histograms_singleton(output, d_false, d_false, e_pdf_page_last, "HIST", "TTTTTT", charts->eta, charts->etas[0], charts->etas[1],
  //		charts->etas[2], charts->etas[3], charts->etas[4]);
}

int main (int argc, char *argv[]) {
  struct s_data_charts charts = {
    d_chart("Number of channel;# Channels;# Entries", 40, 0.0, 40.0),
    d_chart("Number of clusters per event;# Clusters;# Entries", 50.0, 0, 50.0),
    d_chart("Common noise;CN;# Entries", 1200, -60.0, 60.0),
    d_chart("Signal of cluster (foreach event, foreach cluster);Signal;# Entries", 2000, 0.0, 400.0),
    d_chart("sqrt(Signal)/sqrt(MIP);MIP;# Entries", 80.0, 0.0, 20.0),
    {
      d_chart("Signal of clusters with #strips == 1", 2000, 0.0, 400.0),
      d_chart("Signal of clusters with #strips == 2", 2000, 0.0, 400.0),
      d_chart("Signal of clusters with #strips == 3", 2000, 0.0, 400.0),
      d_chart("Signal of clusters with #strips == 4", 2000, 0.0, 400.0),
      d_chart("Signal of clusters with #strips == 5", 2000, 0.0, 400.0)
    },
    d_chart("Signal over noise (SN) of clusters;SN;# Entries", 2000, 0.0, 100.0),
    d_chart("Center of gravity;Gravity;# Entries", d_trb_event_channels, 0.0, d_trb_event_channels),
    d_chart("Center of gravity of SEED strip;Gravity;# Entries", d_trb_event_channels, 0.0, d_trb_event_channels),
    d_chart("ETA;ETA;# Entries", 500, 0.0, 1.0),
    {
      d_chart("ETA of clusters with #strips == 1", 500, 0.0, 100.0),
      d_chart("ETA of clusters with #strips == 2", 500, 0.0, 100.0),
      d_chart("ETA of clusters with #strips == 3", 500, 0.0, 100.0),
      d_chart("ETA of clusters with #strips == 4", 500, 0.0, 100.0),
      d_chart("ETA of clusters with #strips == 5", 500, 0.0, 100.0)
    },
    d_chart("Signal over noise (SN) of clusters with #strips == 1", 5000, 0.0, 100.0),
    d_chart("Signal over noise (SN) of clusters with #strips == 2", 5000, 0.0, 100.0),
    d_chart("Signal over noise (SN) of SEED strip in clusters with #strips == 2", 5000, 0.0, 100.0),
    d_chart("Signal over noise (SN) of SECONDARY strip in clusters with #strips == 2", 5000, 0.0, 100.0),
    d_chart("Signal of clusters with #strips == 1", 2000, 0.0, 400.0),
    d_chart("Signal of clusters with #strips == 2", 2000, 0.0, 400.0),
    d_chart("Signal of SEED strip in clusters with #strips == 2", 2000, 0.0, 400.0),
    d_chart("Signal of SECONDARY strip in clusters with #strips == 2", 2000, 0.0, 400.0),
    {
      d_chart("ETA of clusters with signal 18.0-30.0;ETA;# Entries", 500, 0.0, 1.0),
      d_chart("ETA of clusters with signal 30.0-50.0;ETA;# Entries", 500, 0.0, 1.0),
      d_chart("ETA of clusters with signal 50.0-70.0;ETA;# Entries", 500, 0.0, 1.0),
      d_chart("ETA of clusters with signal 70.0-90.0;ETA;# Entries", 500, 0.0, 1.0),
      d_chart("ETA of clusters with signal 90.0-110.0;ETA;# Entries", 500, 0.0, 1.0)
    },
    d_chart_2D("Signal over ETA;ETA;Signal", 100, 0.0, 1.0, 300.0, 0.0, 300.0),
    d_chart_2D("Signal over gravity;CoG;Signal", d_trb_event_channels, 0.0, d_trb_event_channels, 2000.0, 0.0, 400.0),
    d_chart_2D("sqrt(Signal)/sqrt(MIP) over gravity;CoG;MIP", d_trb_event_channels, 0.0, d_trb_event_channels, 80.0, 0.0, 40.0),
    d_chart_2D("Number of channels over gravity;CoG;NoC", d_trb_event_channels, 0.0, d_trb_event_channels, 40.0, 0.0, 40.0),
    d_chart_2D("Correlation between CoG;CoG #1;CoG #2", d_trb_event_channels, 0.0, d_trb_event_channels, d_trb_event_channels, 0.0, 
        d_trb_event_channels),
    d_chart_2D("Correlation between signals;Signal #1;Signal #2", 80.0, 0.0, 40.0, 80.0, 0.0, 40.0),
    {
      d_chart_2D("Correlation between signals (dampe #1 - AMS #1);Signal dampe #1;Signal AMS #1", 80.0, 0.0, 40.0, 80.0, 0.0, 40.0),
      d_chart_2D("Correlation between signals (dampe #2 - AMS #1);Signal dampe #2;Signal AMS #1", 80.0, 0.0, 40.0, 80.0, 0.0, 40.0),
      d_chart_2D("Correlation between signals (dampe #1 - AMS #2);Signal dampe #1;Signal AMS #2", 80.0, 0.0, 40.0, 80.0, 0.0, 40.0),
      d_chart_2D("Correlation between signals (dampe #2 - AMS #2);Signal dampe #2;Signal AMS #2", 80.0, 0.0, 40.0, 80.0, 0.0, 40.0),
      d_chart_2D("Correlation between signals (AMS #1 - AMS #2); Signal AMS #1; Signal AMS #2", 80.0, 0.0, 40.0, 80.0, 0.0, 40.0)
    },
    d_chart_2D("Correlation between CoG (dampe #1 - AMS #1);CoG dampe #1;CoG AMS #1", d_trb_event_channels, 0.0, d_trb_event_channels,
        d_AMS_channels, 0.0, d_AMS_channels),
    NULL,
    NULL,
    NULL,
    NULL
  };
  struct o_stream *compressed_stream;
  struct o_string *compressed = NULL, *compressed_buffer = NULL, *compressed_list = NULL, *compressed_merged = NULL, *output = NULL, *output_root = NULL, 
                  *extension = f_string_new_constant(NULL, ".root"), *compressed_ams = NULL;
  struct s_exception *exception = NULL;
  int arguments = 0, index;
  float range_start, range_end;
  TFile *output_file;
  d_try {
    d_compress_argument(arguments, "-c", compressed, d_string_pure, "No compressed file specified (-c)");
    d_compress_argument(arguments, "-cl", compressed_list, d_string_pure, "No compressed file list specified (-cl)");
    d_compress_argument(arguments, "-cm", compressed_merged, d_string_pure, "No compressed (multi file) specified (-cm)");
    d_compress_argument(arguments, "-o", output, d_string_pure, "No output file specified (-o)");
    d_compress_argument(arguments, "-porchetta", compressed_ams, d_string_pure, "No AMS compressed file specified (-porchetta)");
    d_compress_argument(arguments, "-shift", v_shift_dampe, atoi, "No Dampe event shift factor specified (ex: -shift 3 -> the third Dampe event is the first one)");
    d_compress_argument(arguments, "-shift_ams", v_shift_ams, atoi, "No AMS event shift factor specified (ex: -shift_ams 3 -> the third AMS event is the first one)");
    if (((compressed) || (compressed_list) || (compressed_merged)) && (output)) {
      output_root = d_clone(output, struct o_string);
      output_root->m_append(output_root, extension);
      output_file = new TFile(output_root->content, "RECREATE");
      d_release(output_root);
      output_file->cd();
      if (compressed_ams) {
        f_load_ams(compressed_ams);
        printf("AMS file with %lld events\n", v_entries);
      }
      if (compressed_list) {
        compressed_stream = f_stream_new_file(NULL, compressed_list, "r", 0777);
        while ((compressed = compressed_stream->m_read_line(compressed_stream, compressed_buffer, d_string_buffer_size))) {
          f_fill_histograms(compressed, &charts);
          compressed_buffer = compressed;
        }
        d_release(compressed_stream);
      } else if (compressed)
        f_fill_histograms(compressed, &charts);
      else {
        v_correlation = true;
        f_fill_histograms(compressed_merged, &charts);
      }
      charts.profile = (TH1D *) charts.signal_eta->ProfileX("Signal over eta (Profile)");
      charts.profile_sg = (TH1D *) charts.signal_gravity->ProfileX("Signal over gravity (Profile)");
      charts.profile_sgm = (TH1D *) charts.signal_gravity_MIP->ProfileX("sqrt(Signal)/sqrt(MIP) over gravity (Profile)");
      charts.profile_nc_g = (TH1D *) charts.n_channels_gravity->ProfileX("Number of channels over gravity (Profile)");
      range_start = charts.signal_over_noise->GetMean()-5.0*charts.signal_over_noise->GetRMS();
      range_end = charts.signal_over_noise->GetMean()+10.0*charts.signal_over_noise->GetRMS();
      charts.signal_over_noise->GetXaxis()->SetRangeUser(range_start, range_end);
      charts.channel_one->GetXaxis()->SetRangeUser(range_start, range_end);
      charts.channels_two->GetXaxis()->SetRangeUser(range_start, range_end);
      charts.channels_two_major->GetXaxis()->SetRangeUser(range_start, range_end);
      charts.channels_two_minor->GetXaxis()->SetRangeUser(range_start, range_end);
      range_start = charts.signals->GetMean()-5.0*charts.signals->GetRMS();
      range_end = charts.signals->GetMean()-10.0*charts.signals->GetRMS();
      charts.signals->GetXaxis()->SetRangeUser(range_start, range_end);
      for (index = 0; index < 5; index++)
        charts.signals_array[index]->GetXaxis()->SetRangeUser(range_start, range_end);
      charts.signal_one->GetXaxis()->SetRangeUser(range_start, range_end);
      charts.signals_two->GetXaxis()->SetRangeUser(range_start, range_end);
      charts.signals_two_major->GetXaxis()->SetRangeUser(range_start, range_end);
      charts.signals_two_minor->GetXaxis()->SetRangeUser(range_start, range_end);
      f_export_histograms(output, &charts);
      output_file->Write();
      output_file->Close();
      delete output_file;
    } else
      d_log(e_log_level_ever, "Missing arguments", NULL);
    d_release(compressed);
    d_release(output);
  } d_catch(exception) {
    d_exception_dump(stderr, exception);
  } d_endtry;
  d_release(extension);
  return 0;
}

