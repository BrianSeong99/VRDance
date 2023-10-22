import json

dataset_name = "jm-2"

# this order cannot be changed, unity.json must be the first one
filenames = [
    'unity.json',
    'rforearm.json',
    'rupperarm.json',
]

current_body_part = "RightArm"

body_parts = ["RightArm"]
unity_features = {
    "RightArm": [
        # "rForearmBendPosX", "rForearmBendPosY", "rForearmBendPosZ", 
        "rForearmBendX", "rForearmBendY", "rForearmBendZ", 
        "rForearmBendRotX", "rForearmBendRotY", "rForearmBendRotZ", "rForearmBendRotW", 

        # "rHandPosX", "rHandPosY", "rHandPosZ",
        "rHandX", "rHandY", "rHandZ",
        "rHandRotX", "rHandRotY", "rHandRotZ", "rHandRotW", 
         
        # "rMid1PosX", "rMid1PosY", "rMid1PosZ", 
        "rMid1X", "rMid1Y", "rMid1Z", 
        "rMid1RotX", "rMid1RotY", "rMid1RotZ", "rMid1RotW",

        # "rShldrBendPosX", "rShldrBendPosY", "rShldrBendPosZ", 
        "rShldrBendX", "rShldrBendY", "rShldrBendZ", 
        "rShldrBendRotX", "rShldrBendRotY", "rShldrBendRotZ", "rShldrBendRotW",

        # "rThumb2PosX", "rThumb2PosY", "rThumb2PosZ", 
        "rThumb2X", "rThumb2Y", "rThumb2Z", 
        "rThumb2RotX", "rThumb2RotY", "rThumb2RotZ", "rThumb2RotW", 
    ]
}
tracker_features = ["AX", "AY", "AZ", "GX", "GY", "GZ", "LAX","LAY", "LAZ", "MX", "MY", "MZ"]

# strategy 1: find average of all moves in 1 seconds
# strategy 2: find the matching timestamps of all three and only keep the matched ones
strategy_choice = 2

data = []
processed_data = []

for i in range(len(filenames)):
    with open("./data/"+dataset_name+"/"+filenames[i], 'r') as f:
        data.append(json.load(f))

if strategy_choice == 1:
    pass
elif strategy_choice == 2:
    key_lists = [list(d.keys()) for d in data]
    # Get the keys of each dictionary and find their intersection
    matching_timestamp = [k for k in key_lists[0] if all(k in kl for kl in key_lists[1:])]

    print("Matching Timestamp size:", len(matching_timestamp))
    
    def unity_to_tensor():
        output = []
        for ts in matching_timestamp:
            current_frame = data[0][ts][current_body_part]
            processed_frame = []
            for feature in unity_features[current_body_part]:
                processed_frame.append(current_frame[feature])
            output.append(processed_frame)
        with open("./processed_data/"+dataset_name+"/unity.json", 'w') as f:
            print("./processed_data/"+dataset_name+"/unity.json", "Data Size: ", len(output))
            json.dump(output, f)
    
    def trackers_to_tensor(tracker_index):
        output = []
        for ts in matching_timestamp:
            current_frame = data[tracker_index][ts]
            processed_frame = []
            for feature in tracker_features:
                processed_frame.append(float(current_frame[feature]))
            output.append(processed_frame)
        with open("./processed_data/"+dataset_name+"/"+filenames[tracker_index], 'w') as f:
            print("./processed_data/"+dataset_name+"/"+filenames[tracker_index], "Data Size: ", len(output))
            json.dump(output, f)

    unity_to_tensor()
    for i in range(1, len(filenames)):
        trackers_to_tensor(i)


    