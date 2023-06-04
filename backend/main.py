import json
import time
import requests
import torch
from torch import nn
from torch.utils.data import Dataset, DataLoader, random_split
import pytorch_lightning as pl
import firebase_admin
# from firebase_admin import db

class LSTMModel(pl.LightningModule):
    def __init__(self, input_size, hidden_size, output_size, num_layers):
        super(LSTMModel, self).__init__()
        self.lstm1 = nn.LSTM(input_size, hidden_size, num_layers, batch_first=True)
        self.lstm2 = nn.LSTM(input_size, hidden_size, num_layers, batch_first=True)
        self.fc = nn.Linear(hidden_size * 2, output_size)
        self.hidden_1 = None
        self.hidden_2 = None
        self.is_inference = False

    def forward(self, x1, x2):
        output1, (h1, c1) = self.lstm1(x1, self.hidden_1 if self.is_inference else None)
        output2, (h2, c2) = self.lstm2(x2, self.hidden_2 if self.is_inference else None)
        self.hidden_1 = (h1.detach(), c1.detach())
        self.hidden_2 = (h2.detach(), c2.detach())

        output = torch.cat((output1, output2), dim=-1)
        output = self.fc(output)
        return output

    def training_step(self, batch, batch_idx):
        x1, x2, y = batch
        y_hat = self.forward(x1, x2)
        loss = nn.MSELoss()(y_hat, y)
        self.log('train_loss', loss)
        return loss

    def validation_step(self, batch, batch_idx):
        x1, x2, y = batch
        y_hat = self.forward(x1, x2)
        val_loss = nn.MSELoss()(y_hat, y)
        self.log('val_loss', val_loss, prog_bar=True)

    def configure_optimizers(self):
        return torch.optim.Adam(self.parameters(), lr=0.01)


model = LSTMModel(input_size=12, hidden_size=128, output_size=35, num_layers=2)
model.is_inference = True
checkpoint_path = "lstm_model_jm-1-3.pth"
print("./models/" + checkpoint_path)
model.load_state_dict(torch.load("./models/" + checkpoint_path))

model.eval()

firebase_url_RUpperArm = "https://vrdance-rupperarm-default-rtdb.firebaseio.com/"
firebase_url_RForeArm = "https://vrdance-rforearm-default-rtdb.firebaseio.com/"

# cred = firebase_admin.credentials.Certificate("./vrdance-rforearm-firebase-adminsdk-14zzw-9a3c82493a.json")
# default_app = firebase_admin.initialize_app(cred, {'databaseURL':firebase_url_RForeArm})

# # Get a database reference to our posts
# ref = db.reference("/")

# # Read the data at the posts reference (this is a blocking operation)
# print(ref.get())

while True:
    # response_RUpperArm = requests.get(firebase_url_RUpperArm+'/.json?orderBy=\"$key\"&limitToLast=1')
    response_RForeArm = requests.get(firebase_url_RForeArm+'/.json?orderBy=\"$key\"&limitToLast=1')

    # print(response_RUpperArm.text)
    # print(response_RForeArm.text)
    print()

    # data_RUpperArm = response_RUpperArm.json()
    data_RForeArm = response_RForeArm.json()

    print(data_RForeArm)
    
    # time.sleep(0.25)  # Delay for 250 milliseconds
    # with torch.no_grad():
    #     output, = model(x1_tensor, x2_tensor)
