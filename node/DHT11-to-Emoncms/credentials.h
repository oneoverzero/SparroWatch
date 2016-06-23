/*
   Personal definitions
*/

#define ssid     "your_ap_ssid"
#define password "your_ap_password"
#define host     "emoncms.org"
#define apikey   "your_apikey"
#define node     "1"

/*
   Helper defines
*/
#define baseurl  "/input/post.json?apikey=" + String(apikey) + "&node=" + node + "&json="