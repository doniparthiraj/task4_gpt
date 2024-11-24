import torch
from torch.utils.data import Dataset, DataLoader
from transformers import GPT2LMHeadModel, GPT2Tokenizer, AdamW

# Initialize the Tokenizer
tokenizer = GPT2Tokenizer.from_pretrained('gpt2')

# Set the padding token to the EOS token
tokenizer.pad_token = tokenizer.eos_token

# Custom Dataset Class
class AirlineDataset(Dataset):
    def __init__(self, tokenizer, data, max_length=512):
        self.tokenizer = tokenizer
        self.data = data
        self.max_length = max_length

    def __len__(self):
        return len(self.data)

    def __getitem__(self, idx):
        text = self.data[idx]
        encoding = self.tokenizer(
            text,
            return_tensors='pt',
            truncation=True,
            padding='max_length',
            max_length=self.max_length
        )
        input_ids = encoding['input_ids'].flatten()
        attention_mask = encoding['attention_mask'].flatten()

        return {
            'input_ids': input_ids,
            'attention_mask': attention_mask
        }

# Load Data from FAQs.txt
def load_data_from_file(file_path):
    data = []
    with open(file_path, 'r') as file:
        for line in file:
            line = line.strip()
            if line:
                # Split the line into the user query and bot response
                user_query, bot_response = line.split("|||")
                # Format the data as a conversation prompt
                formatted_text = f"User: {user_query.strip()} bot: {bot_response.strip()}"
                data.append(formatted_text)
    return data


# Load and Prepare Data
data_file_path = 'FAQs.txt'
data = load_data_from_file(data_file_path)

# Prepare the Dataset and DataLoader
train_dataset = AirlineDataset(tokenizer, data)
train_loader = DataLoader(train_dataset, batch_size=2, shuffle=True)

# Initialize the Model
model = GPT2LMHeadModel.from_pretrained('gpt2')

# Fine-Tuning the Model
optimizer = AdamW(model.parameters(), lr=5e-5)
model.train()

for epoch in range(3):  # Adjust the number of epochs as needed
    for batch in train_loader:
        optimizer.zero_grad()
        outputs = model(input_ids=batch['input_ids'], attention_mask=batch['attention_mask'], labels=batch['input_ids'])
        loss = outputs.loss
        loss.backward()
        optimizer.step()
        print(f"Epoch {epoch + 1}, Loss: {loss.item()}")

# Save the Fine-Tuned Model
model.save_pretrained('./fine_tuned_gpt2_airline')
tokenizer.save_pretrained('./fine_tuned_gpt2_airline')
